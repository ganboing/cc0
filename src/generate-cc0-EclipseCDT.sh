#!/bin/bash

dir=$(dirname "$0")

mkdir -p $dir/../../cc0-EclipseCDT

cd $dir/../../cc0-EclipseCDT

# Accept an optional command line parameter to set the build type.
# Valid build types are Debug, Release, RelWithDebInfo, MinSizeRel
if [ -z "$1" ]; then
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$dir/../cc0-install -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=$1 -DCC0_USE_STATIC_CRT=1 $dir/../cc0/src/toolchain/
else
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$dir/../cc0-install -G"Eclipse CDT4 - Unix Makefiles" -DCC0_USE_STATIC_CRT=1 $dir/../cc0/src/toolchain
fi

# make VERBOSE=1
cd -

