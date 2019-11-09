#!/bin/bash

rm CMakeCache.txt -f
cmake ./

while getopts rR opt
do
	case $opt in
		r)
			make clean
			;;
		R)
			make clean
			;;
		*)
			;;
	esac
done

make -j 4

cd $root_dir