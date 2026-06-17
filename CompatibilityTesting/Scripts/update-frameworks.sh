#!/bin/zsh

set -e

SCRIPT_DIR="${0:A:h}"
ROOT_DIR="${SCRIPT_DIR:h:h}"
RESOURCES_DIR="$SCRIPT_DIR/Resources"

XCFRAMEWORK_DIR="$ROOT_DIR/CompatibilityTesting/Frameworks/AttributeGraph.xcframework"

swift build --package-path "$ROOT_DIR" --target Compute -c release
BIN_PATH=$(swift build --package-path "$ROOT_DIR" --target Compute -c release --show-bin-path)
SWIFT_INTERFACE_FILE="$BIN_PATH/Modules/Compute.swiftinterface"

MACOS_VERSION=$(xcrun --sdk macosx --show-sdk-version)
IOS_VERSION=$(xcrun --sdk iphoneos --show-sdk-version)

copy_info_plist() {
    local dest_dir="$1"
    
    cat > "$dest_dir/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>AttributeGraph</string>
	<key>CFBundleIdentifier</key>
	<string>com.apple.AttributeGraph</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>FMWK</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0.0</string>
</dict>
</plist>
EOF

}

copy_headers() {
    local dest_dir="$1"

    mkdir -p "$dest_dir"
    for src in "$ROOT_DIR"/Sources/ComputeCxx/include/ComputeCxx/*.h; do
        local filename=$(basename "$src")
        local dest_filename="${filename/ComputeCxx/AttributeGraph}"
        dest_filename="${dest_filename/IAG/AG}"
        sed \
            -e 's/ComputeCxx/AttributeGraph/g' \
            -e 's/IAG/AG/g' \
            "$src" > "$dest_dir/$dest_filename"
    done
}

copy_modules() {
    local dest_dir="$1"
    shift

    mkdir -p "$dest_dir"    
    cat > "$dest_dir/module.modulemap" << EOF
framework module AttributeGraph [system] {
    umbrella header "AttributeGraph.h"

    export *
    module * {
        export *
    }
}
EOF

    local swiftmodule_dir="$dest_dir/AttributeGraph.swiftmodule"
    mkdir -p "$swiftmodule_dir"
    for triple in "$@"; do
        local swiftinterface_filename=$(echo "$triple" | sed 's/macos[0-9][0-9.]*/macos/; s/ios[0-9][0-9.]*/ios/')
        sed \
            -e "s/-target [^ ]*/-target $triple/" \
            -e 's/ -package-name [^ ]*//' \
            -e 's/ComputeCxx/AttributeGraph/g' \
            -e 's/Compute/AttributeGraph/g' \
            "$SWIFT_INTERFACE_FILE" > "$swiftmodule_dir/${swiftinterface_filename}.swiftinterface"
    done
}

copy_tbd_file() {
    local dest_dir="$1"

    cp "$RESOURCES_DIR/AttributeGraph.tbd" "$dest_dir/AttributeGraph.tbd"
}

update_library() {
    local library_identifier="$1"
    shift

    local library_path="$XCFRAMEWORK_DIR/$library_identifier/AttributeGraph.framework"
    rm -rf "$library_path"

    if [[ "$1" == *-apple-macos* ]]; then
        local version_dir="$library_path/Versions/A"
        mkdir -p "$version_dir"
        mkdir -p "$version_dir/Resources"

        copy_info_plist "$version_dir/Resources"
        copy_headers "$version_dir/Headers"
        copy_modules "$version_dir/Modules" "$@"
        copy_tbd_file "$version_dir"

        ln -s A "$library_path/Versions/Current"
        ln -s Versions/Current/Headers "$library_path/Headers"
        ln -s Versions/Current/Modules "$library_path/Modules"
        ln -s Versions/Current/Resources "$library_path/Resources"
        ln -s Versions/Current/AttributeGraph.tbd "$library_path/AttributeGraph.tbd"
    else
        mkdir -p "$library_path"

        copy_info_plist "$library_path"
        copy_headers "$library_path/Headers"
        copy_modules "$library_path/Modules" "$@"
        copy_tbd_file "$library_path"
    fi

    echo "Updated library: $library_identifier"
}

update_library "macos-arm64_arm64e_x86_64" \
    "arm64-apple-macos${MACOS_VERSION}" \
    "arm64e-apple-macos${MACOS_VERSION}" \
    "x86_64-apple-macos${MACOS_VERSION}"

update_library "maccatalyst-arm64_arm64e_x86_64" \
    "arm64-apple-ios${IOS_VERSION}-macabi" \
    "arm64e-apple-ios${IOS_VERSION}-macabi" \
    "x86_64-apple-ios${IOS_VERSION}-macabi"
