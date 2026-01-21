#!/bin/sh

set -e

if [ -z "$1" ]; then
    echo "please provide a target project!"
    exit 1
fi

# compile
scons target=template_debug debug_symbols=yes dev_build=yes

# copy to project
echo "removing $1/bin if it exists ..."
rm -rf "$1/bin"

echo "copying library and .gdextension to $1/bin ..."
mkdir -p "$1/bin" && cp -rf demo/bin/ "$1/bin" && cp gdexample.gdextension "$1/bin"

echo "\nbuild complete!"