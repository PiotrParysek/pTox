#!/bin/sh
sudo apt update
sudo apt upgrade -y
sudo apt install -y libsodium-dev libm17n-0 libpthread-stubs0-dev librt-client-rest-perl libopus-dbg libvpx-dev cmake gcc build-essential
wget https://github.com/TokTok/c-toxcore/archive/v0.2.0.tar.gz
tar -xf
cd c-toxcore-0.2.0
mkdir _build
cd _build
cmake ..
make
sudo make install
