
./clean_all_obj.sh

root_dir=`pwd`

all_dir=("./servers/common/build" "./servers/lobby_server/build" "./servers/game_server/build" 
		 "./servers/land_robot_ai_ex/build" "./servers/land_robot_server/build" "./servers/bainiu_server/build"
		 "./servers/zajinhua_server/build" "./servers/niuniu_server/build" "./servers/dice_server/build"
		 "./servers/baccarat_server/build" "./servers/paijiu_server/build" "./servers/showhand_server/build"
		  "./servers/texas_server/build" "./servers/everycolor_server/build" "./servers/majiang_server/build"
		  "./servers/slot_server/build" "./servers/war_server/build" "./servers/fight_server/build"
		  "./servers/robniu_server/build" "./servers/twoeight_server/build" "./servers/carcity_server/build")


tLen=${#all_dir[@]}
while getopts rR opt
do
	case $opt in
		r)
			for ((i=0;i<${tLen};i++))
			do
				dirname=${all_dir[$i]}
				cd $dirname
				./rebuild.sh -r;
				cd $root_dir;
			done
			
            exit 0
            ;;
		*)
			;;
	esac
done
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./rebuild.sh;
	cd $root_dir;
done




