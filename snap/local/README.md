# Instructions for snap packaging

1. Compile dictionary files: `cd src/zxcvbn-c && make && cd ../..`
2. Run `wget https://github.com/blur-network/blur/releases/download/v0.1.9.9.2/blur-v0.1.9.9.4-linux-x86_64.tar.gz`, to get static blurd bin, for latest release.
3. Then extract, and place in `blur-network-gui` root directory: `tar xvzf blur-v0.1.9.9.4-linux-x86_64.tar.gz && cp blur-v0.1.9.9.4-linux-x86_64/blurd ./ && rm -rf blur-v0.1.9.9.4-linux-x86_64`
4. Build with snapcraft:  `snapcraft snap`
