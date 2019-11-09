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

rm -f ../../../release/two_people_majiang_server/majiangServer
cp ../../../release/majiang_server/majiangServer ../../../release/two_people_majiang_server/majiangServer


cd $root_dir
