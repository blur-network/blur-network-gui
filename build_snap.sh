#!/bin/bash

BUILD_TYPE=$1
source ${SNAPCRAFT_PART_SRC}/utils.sh
platform=$(get_platform)
# default build type
if [ -z $BUILD_TYPE ]; then
    BUILD_TYPE=release
fi

# Return 0 if the command exists, 1 if it does not.
exists() {
    command -v "$1" &>/dev/null
}

# Return the first value in $@ that's a runnable command.
find_command() {
    for arg in "$@"; do
        if exists "$arg"; then
           echo "$arg"
           return 0
        fi
    done
    return 1
}

if [ "$BUILD_TYPE" == "release" ]; then
    echo "Building release"
    CONFIG="CONFIG+=release";
    BIN_PATH=release/bin
elif [ "$BUILD_TYPE" == "release-static" ]; then
    echo "Building release-static"
    if [ "$platform" != "darwin" ]; then
	    CONFIG="CONFIG+=release static";
    else
        # OS X: build static libwallet but dynamic Qt. 
        echo "OS X: Building Qt project without static flag"
        CONFIG="CONFIG+=release";
    fi    
    BIN_PATH=release/bin
elif [ "$BUILD_TYPE" == "release-android" ]; then
    echo "Building release for ANDROID"
    CONFIG="CONFIG+=release static WITH_SCANNER DISABLE_PASS_STRENGTH_METER";
    ANDROID=true
    BIN_PATH=release/bin
    DISABLE_PASS_STRENGTH_METER=true
elif [ "$BUILD_TYPE" == "debug-android" ]; then
    echo "Building debug for ANDROID : ultra INSECURE !!"
    CONFIG="CONFIG+=debug qml_debug WITH_SCANNER DISABLE_PASS_STRENGTH_METER";
    ANDROID=true
    BIN_PATH=debug/bin
    DISABLE_PASS_STRENGTH_METER=true
elif [ "$BUILD_TYPE" == "debug" ]; then
    echo "Building debug"
	CONFIG="CONFIG+=debug"
    BIN_PATH=debug/bin
else
    echo "Valid build types are release, release-static, release-android, debug-android and debug"
    exit 1;
fi


source ${SNAPCRAFT_PART_SRC}/utils.sh
pushd $(pwd)
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
MONERO_DIR=monero
MONEROD_EXEC=blurd

MAKE='make'
if [[ $platform == *bsd* ]]; then
    MAKE='gmake'
fi

# build libwallet
${SNAPCRAFT_PART_SRC}/get_libwallet_api.sh $BUILD_TYPE

make -C '${SNAPCRACFT_PART_SRC}/src/zxcvbn-c'

if [ ! -d build ]; then mkdir build; fi


# Platform indepenent settings
if [ "$ANDROID" != true ] && ([ "$platform" == "linux32" ] || [ "$platform" == "linux64" ]); then
    exists lsb_release && distro="$(lsb_release -is)"
    if [ "$distro" = "Ubuntu" ] || [ "$distro" = "Fedora" ] || test -f /etc/fedora-release; then
        CONFIG="$CONFIG libunwind_off"
    fi
fi

if [ "$platform" == "darwin" ]; then
    BIN_PATH=$BIN_PATH/blur-gui-wallet.app/Contents/MacOS/
elif [ "$platform" == "mingw64" ] || [ "$platform" == "mingw32" ]; then
    MONEROD_EXEC=blurd.exe
fi

cd build
QMAKE='qmake'

$QMAKE ${SNAPCRAFT_PART_SRC}/blur-gui-wallet.pro "$CONFIG" || exit
$MAKE || exit

# Copy blurd to bin folder
if [ "$platform" != "mingw32" ] && [ "$ANDROID" != true ]; then
cp ${SNAPCRAFT_PART_BUILD}/$MONERO_DIR/bin/$MONEROD_EXEC ${SNAPCRAFT_PART_BUILD}/build/release/bin/
fi

make deploy
popd


