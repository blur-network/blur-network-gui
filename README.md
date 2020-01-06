# Blur Network GUI Wallet

![Blur Network Wallet](https://cdn.discordapp.com/attachments/453123992736366594/553617741760692234/gui.png)

Copyright (c) 2018-2019, The Blur Network</br> 
Copyright (c) 2014-2018, The Monero Project

## License

See [LICENSE](LICENSE).

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/blur-wallet)


[![blur-wallet](https://snapcraft.io//blur-wallet/badge.svg)](https://snapcraft.io/blur-wallet)


## Compiling the Blur Network GUI Wallet from source

### On Linux:

(Tested on Ubuntu 18.04 x64)

1. Install Monero dependencies

  - For Debian distributions (Debian, Ubuntu, Mint, Tails...)

	`sudo apt install build-essential cmake libboost-all-dev libsodium-dev libunwind8-dev pkg-config libssl-dev`



2. Install Qt:

  *Note*: Qt 5.7 is the minimum version required to build the GUI. This makes **some** distributions (mostly based on debian, like Ubuntu 16.x or Linux Mint 18.x) obsolete. You can still build the GUI if you install an [official Qt release](https://wiki.qt.io/Install_Qt_5_on_Ubuntu), but this is not officially supported.

  - For Ubuntu 17.10+

    `sudo apt install qtbase5-dev qt5-default qtdeclarative5-dev qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-dialogs qml-module-qtquick-xmllistmodel qml-module-qt-labs-settings qml-module-qt-labs-folderlistmodel qttools5-dev-tools qml-module-qtquick-templates2`


  - Optional : To build the flag `WITH_SCANNER`

    - For Ubuntu

      `sudo apt install qtmultimedia5-dev qml-module-qtmultimedia libzbar-dev`


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
	
Using the `./start-gui.sh` script will start a daemon in the background connecting to the seed nodes in the network, with a p2p port at 52541 and rpc on 52542.  The daemon will run in the background and only be interactive through the GUI interface.  Closing the GUI wallet and clicking 'stop daemon' will terminate the daemon.
