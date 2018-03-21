#!/bin/sh
sudo apt update
sudo apt upgrade -y
sudo apt install -y libsodium-dev libm17n-0 libpthread-stubs0-dev librt-client-rest-perl libopus-dbg libvpx-dev
wget https://github.com/TokTok/c-toxcore/archive/v0.2.1.zip -O c-toxcore_v0.2.1.zip
unzip c-toxcore_v0.2.1.zip
cd c-toxcore-0.2.1
mkdir _build
cd _build
cmake ..
make
sudo make install
