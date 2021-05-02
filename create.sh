#!/usr/bin/env bash
cd /home/michaelb/dev/coal
./create.sh
cd -
cd /home/michaelb/dev/hell
./create.sh
cd -
cd build
sudo make install
cd ..
