#!/bin/bash

root_dir=`pwd`
./stop_all.sh

all_dir=("./land_server/" "./bainiu_server/" "./zajinhua_server/" "./niuniu_server" "./baccarat_server"
         "./paijiu_server" "./showhand_server" "./texas_server" "./dice_server" "./everycolor_server/"
		 "./two_people_majiang_server" "./slot_server" "./war_server" "./fight_server" "./robniu_server"
		 "./fishing_server" "./twoeight_server" "./carcity_server")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./restart_game.sh;
	cd $root_dir;
done

sleep 5

cd $root_dir
cd ./lobby_server/
./restart_game.sh




