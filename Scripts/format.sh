#!/bin/zsh

set -e

echo "Formatting Swift sources in $(pwd)"

# Run the format / lint commands
git ls-files -z '*.swift' | xargs -0 swift format format --parallel --in-place
git ls-files -z '*.swift' | xargs -0 swift format lint --strict --parallel