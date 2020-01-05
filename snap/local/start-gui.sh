#!/bin/bash
export LD_LIBRARY_PATH=`pwd`/libs
export QT_PLUGIN_PATH=`pwd`/plugins
export QML2_IMPORT_PATH=`pwd`/qml
# make it so that it can be called from anywhere and also through soft links
SCRIPT_DIR="$(dirname "$(test -L "${BASH_SOURCE[0]}" && readlink "${BASH_SOURCE[0]}" || echo "${BASH_SOURCE[0]}")")"
"$SCRIPT_DIR"/blur-gui-wallet "$@"
