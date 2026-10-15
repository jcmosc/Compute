#!/bin/bash
#
# Run clang-tidy on C++ source files.
# Usage: ./scripts/run-clang-tidy.sh [run-clang-tidy.py options]
#
# Examples:
#   ./scripts/run-clang-tidy.sh           # Check all files
#   ./scripts/run-clang-tidy.sh -fix      # Apply fixes
#   ./scripts/run-clang-tidy.sh -quiet    # Less output

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/.build/clang-tidy"

mkdir -p "$BUILD_DIR"

# System include path (macOS or Linux)
if command -v xcrun &>/dev/null; then
    SYSROOT="-isysroot$(xcrun --show-sdk-path)"
else
    SYSROOT=""
fi

CXXFLAGS="-std=c++20 $SYSROOT \
-I$PROJECT_ROOT/Sources \
-I$PROJECT_ROOT/Sources/ComputeCxx \
-I$PROJECT_ROOT/Sources/ComputeCxx/include \
-I$PROJECT_ROOT/Sources/ComputeCxx/internalInclude \
-I$PROJECT_ROOT/Sources/Platform \
-I$PROJECT_ROOT/Sources/Platform/include \
-I$PROJECT_ROOT/Sources/Utilities \
-I$PROJECT_ROOT/Sources/Utilities/include \
-isystem$PROJECT_ROOT/Submodules/swift-runtime-headers/include \
-isystem$PROJECT_ROOT/Submodules/swift-runtime-headers/stdlib/include \
-DCOMPILED_WITH_SWIFT -DPURE_BRIDGING_MODE"

# Generate compile_commands.json for all C++ targets
{
    echo "["
    sep=""
    find "$PROJECT_ROOT/Sources/ComputeCxx" \
         "$PROJECT_ROOT/Sources/Platform" \
         "$PROJECT_ROOT/Sources/Utilities" \
         -name '*.cpp' -type f 2>/dev/null | sort | while read -r file; do
        printf '%s\n{"directory":"%s","file":"%s","command":"clang++ %s -c %s"}' \
            "$sep" "$PROJECT_ROOT" "$file" "$CXXFLAGS" "$file"
        sep=","
    done
    echo "]"
} > "$BUILD_DIR/compile_commands.json"

exec python3 "$SCRIPT_DIR/run-clang-tidy.py" \
    -p="$BUILD_DIR" \
    -config-file="$PROJECT_ROOT/.clang-tidy" \
    -header-filter="$PROJECT_ROOT/Sources/(ComputeCxx|Platform|Utilities)/.*\.h$" \
    "$@"
