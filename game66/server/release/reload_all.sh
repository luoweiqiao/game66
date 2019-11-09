#!/bin/bash

root_dir=`pwd`

all_dir=("./land_server/" "./bainiu_server/" "./zajinhua_server/" "./niuniu_server" "./baccarat_server"
         "./paijiu_server" "./showhand_server" "./texas_server" "./dice_server" "./majiang_server" "./twoeight_server" "./carcity_server")


tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./reload.sh;
	cd $root_dir;
done




