#!/bin/sh
set -ex

wget http://kent.dl.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.bz2
tar -xjf boost_1_56_0.tar.bz2
cd boost_1_56_0
./bootstrap.sh
./b2 --with-test
BOOST_ROOT=$PWD && export BOOST_ROOT

sudo apt-get update
sudo apt-get install -q -y liblua5.2-dev
