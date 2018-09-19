#!/bin/bash

./blurd --seed-node 178.128.191.245:14894 --seed-node 178.128.186.101:14894 --seed-node 178.128.180.136:14894 --p2p-bind-port 14894 --rpc-bind-ip 127.0.0.1 --rpc-bind-port 14895 --detach --non-interactive && ./blur-gui-wallet
