# Instructions for snap packaging

1.) Run `wget https://github.com/blur-network/blur/releases/download/v0.1.9.9.2/blur-v0.1.9.9.4-linux-x86_64.tar.gz`, to get static blurd bin, for latest release.


2.) Then extract, and place in monero submodule root directory: `tar xvzf blur-v0.1.9.9.4-linux-x86_64.tar.gz && cp blur-v0.1.9.9.4-linux-x86_64/blurd ./monero/ && rm -rf blur-v0.1.9.9.4-linux-x86_64`


3.) Build with snapcraft:  `snapcraft snap`


4.) {{[troutslap](https://en.wikipedia.org/wiki/Template:Trout)}}
