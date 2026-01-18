#!/bin/zsh

set -e

BUILD_DIR=".build/Xcode"

xcodebuild archive \
    -project Compute.xcodeproj \
    -scheme Compute \
    -destination "generic/platform=macOS" \
    -archivePath "$BUILD_DIR/Archives/Compute-macOS.xcarchive" \
    ENABLE_USER_SCRIPT_SANDBOXING=NO

rm -rf "$BUILD_DIR/Frameworks/Compute.xcframework"

xcodebuild -create-xcframework \
    -archive "$BUILD_DIR/Archives/Compute-macOS.xcarchive" -framework Compute.framework \
    -output "$BUILD_DIR/Frameworks/Compute.xcframework"

# Post-process swiftinterface files to replace ComputeCxx references with Compute
find "$BUILD_DIR/Frameworks/Compute.xcframework" -name "*.swiftinterface" | while read -r file; do
    sed -i '' 's/ComputeCxx/Compute/g' "$file"
    echo "Processed: $file"
done

# Copy module.modulemap into each framework's Modules directory
find "$BUILD_DIR/Frameworks/Compute.xcframework" -type d -name "Modules" | while read -r modules_dir; do
    cp Scripts/Resources/module.modulemap "$modules_dir/"
    echo "Copied modulemap to: $modules_dir"
done

# Fix missing Headers symlink
find "$BUILD_DIR/Frameworks/Compute.xcframework" -type d -name "Compute.framework" | while read -r framework_dir; do
    if [ -d "$framework_dir/Versions" ] && [ ! -L "$framework_dir/Headers" ]; then
        ln -s Versions/Current/Headers "$framework_dir/Headers"
        echo "Added Headers symlink to: $framework_dir"
    fi
done
