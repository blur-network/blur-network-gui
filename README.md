# Blur Network GUI Wallet

Copyright (c) 2018, The Blur Network</br> 
Copyright (c) 2014-2018, The Monero Project

## License

See [LICENSE](LICENSE).

## Compiling the Blur Network GUI Wallet from source

### On Linux:

(Tested on Ubuntu 18.04 x64)

1. Install Monero dependencies

  - For Debian distributions (Debian, Ubuntu, Mint, Tails...)

	`sudo apt install build-essential cmake libboost-all-dev miniupnpc libunbound-dev graphviz doxygen libunwind8-dev pkg-config libssl-dev libzmq3-dev libhidapi-dev`

  - For Gentoo

	`sudo emerge app-arch/xz-utils app-doc/doxygen dev-cpp/gtest dev-libs/boost dev-libs/expat dev-libs/openssl dev-util/cmake media-gfx/graphviz net-dns/unbound net-libs/ldns net-libs/miniupnpc net-libs/zeromq sys-libs/libunwind`

2. Install Qt:

  *Note*: Qt 5.7 is the minimum version required to build the GUI. This makes **some** distributions (mostly based on debian, like Ubuntu 16.x or Linux Mint 18.x) obsolete. You can still build the GUI if you install an [official Qt release](https://wiki.qt.io/Install_Qt_5_on_Ubuntu), but this is not officially supported.

  - For Ubuntu 17.10+

    `sudo apt install qtbase5-dev qt5-default qtdeclarative5-dev qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-dialogs qml-module-qtquick-xmllistmodel qml-module-qt-labs-settings qml-module-qt-labs-folderlistmodel qttools5-dev-tools qml-module-qtquick-templates2`

  - For Gentoo

    `sudo emerge dev-qt/qtcore:5 dev-qt/qtdeclarative:5 dev-qt/qtquickcontrols:5 dev-qt/qtquickcontrols2:5 dev-qt/qtgraphicaleffects:5`

  - Optional : To build the flag `WITH_SCANNER`

    - For Ubuntu

      `sudo apt install qtmultimedia5-dev qml-module-qtmultimedia libzbar-dev`

    - For Gentoo

      The *qml* USE flag must be enabled.

      `emerge dev-qt/qtmultimedia:5 media-gfx/zbar`


3. Clone repository

    `git clone https://github.com/blur-network/blur-network-gui.git`

4. Build

    ```
    cd blur-network-gui
    ./build.sh
    ```

The executables can be found in the build/release/bin folder.

5. Start the GUI Wallet

	```
	cd build/release/bin
	./start-gui-wallet.sh
	```
	
Using the `./start-gui-wallet.sh` script will start a daemon in the background connecting to the seed nodes in the network, with a p2p port at 14894 and rpc on 14895.  The daemon will run in the background and only be interactive through the GUI interface.  Closing the GUI wallet and clicking 'stop daemon' will terminate the daemon.
