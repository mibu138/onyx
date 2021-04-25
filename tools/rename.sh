#!/usr/bin/env bash
read -p "Rename files tanto to obsidian in $PWD? (y/n) " -n1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    sed -i "s/#include <tanto/#include <obsidian/
    s/#include \"tanto/#include \"obsidian/
    s/tanto/obdn/g
    s/Tanto/Obdn/g
    s/TANTO/OBDN/g" {*.[ch],*.cpp}
fi
