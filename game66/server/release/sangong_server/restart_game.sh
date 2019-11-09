#!/bin/sh


./stop.sh

sleep 3
./sangongServer --sid 81 --fd 10000 --loglv 0 --logsize 52428800 --logdays 5 --logname "log" --cfg "../server_config/server_config.lua"


