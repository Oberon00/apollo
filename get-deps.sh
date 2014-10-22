#!/bin/sh
set -ex

wget http://kent.dl.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.bz2
tar -xjf boost_1_56_0.tar.bz2
cd boost_1_56_0
./bootstrap.sh
sudo ./b2 install --with-test -d0

sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt-get update -qq -y
sudo apt-get install -q -y liblua5.2-dev libstdc++-4.8-dev
