#!/bin/bash

dir=$(cd $(dirname "$0"); pwd)
cwd=$(pwd)

echo "running from ""$dir"

mkdir -p $cwd/cc0-build

echo "generating to "$cwd"/cc0-EclipseCDT"

cd $cwd/cc0-EclipseCDT

# Accept an optional command line parameter to set the build type.
# Valid build types are Debug, Release, RelWithDebInfo, MinSizeRel
if [ -z "$1" ]; then
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$cwd/cc0-install -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=$1 -DCC0_USE_STATIC_CRT=1 $dir/toolchain/
else
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$cwd/cc0-install -G"Eclipse CDT4 - Unix Makefiles" -DCC0_USE_STATIC_CRT=1 $dir/toolchain/
fi

# make VERBOSE=1
cd -

