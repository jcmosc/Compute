#!/bin/zsh

set -e

# Default values
CHECKOUT_PATH=".build/checkouts/swift"
SWIFT_VERSION=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --path)
            CHECKOUT_PATH="$2"
            shift 2
            ;;
        --swift-version)
            SWIFT_VERSION="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--path <path>] [--swift-version <major.minor>]"
            exit 1
            ;;
    esac
done

# Detect Swift version if not provided
if [ -z "$SWIFT_VERSION" ]; then
    SWIFT_VERSION=$(swift --version 2>/dev/null | grep -oE 'Swift version [0-9]+\.[0-9]+' | grep -oE '[0-9]+\.[0-9]+')
    echo "Detected Swift version: $SWIFT_VERSION"
fi

SWIFT_VERSION_MAJOR=$(echo "$SWIFT_VERSION" | cut -d. -f1)
SWIFT_VERSION_MINOR=$(echo "$SWIFT_VERSION" | cut -d. -f2)

# Remove existing checkout if present
rm -rf "$CHECKOUT_PATH"
mkdir -p "$CHECKOUT_PATH"

# Clone with sparse checkout
git clone --filter=blob:none --no-checkout --depth 1 \
    --branch "release/$SWIFT_VERSION_MAJOR.$SWIFT_VERSION_MINOR" \
    git@github.com:swiftlang/swift.git "$CHECKOUT_PATH"

cd "$CHECKOUT_PATH"

git sparse-checkout set --no-cone \
    include/swift \
    stdlib/include \
    stdlib/public/SwiftShims \
    stdlib/public/runtime/MetadataAllocatorTags.def

git checkout

# Write CMakeConfig.h file
cat > include/swift/Runtime/CMakeConfig.h << EOF
#ifndef SWIFT_RUNTIME_CMAKECONFIG_H
#define SWIFT_RUNTIME_CMAKECONFIG_H
#define SWIFT_VERSION_MAJOR "$SWIFT_VERSION_MAJOR"
#define SWIFT_VERSION_MINOR "$SWIFT_VERSION_MINOR"
#endif
EOF

echo "The Swift repository has been cloned to $CHECKOUT_PATH"
