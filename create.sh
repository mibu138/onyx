#!/usr/bin/env bash
cd /home/michaelb/dev/coal
./create.sh
cd -
cd /home/michaelb/dev/hell
./create.sh
cd -
cd build
cmake ..
sudo make install
cd ..
