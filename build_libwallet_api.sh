#!/bin/bash


# MONERO_URL=https://github.com/blur-network/blur.git
# MONERO_BRANCH=stable
CPU_CORE_COUNT=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
pushd $(pwd)
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source $ROOT_DIR/utils.sh


INSTALL_DIR=$ROOT_DIR/wallet
MONERO_DIR=$ROOT_DIR/monero


mkdir -p $MONERO_DIR/build/release
pushd $MONERO_DIR/build/release

# reusing function from "utils.sh"
platform=$(get_platform)

pushd $MONERO_DIR/build/release-static/src/wallet
make -j4
make install -j4
popd

popd











