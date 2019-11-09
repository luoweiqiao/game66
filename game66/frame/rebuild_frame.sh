
#!/bin/bash

root_dir=`pwd`

rm -f ../server/server_frame/lib/libnet.a
rm -f ../server/server_frame/lib/libsvrlib.a

rm -f ./server_frame/lib/libnet.a
rm -f ./server_frame/lib/libsvrlib.a

cd $root_dir
cd ./server_frame/include/network
make clean 
make

cd $root_dir
cd ./server_frame/build
./rebuild.sh -r

cd $root_dir
mkdir ../server/server_frame/lib/
cp ./server_frame/lib/libnet.a  ../server/server_frame/lib/libnet.a
cp ./server_frame/lib/libsvrlib.a  ../server/server_frame/lib/libsvrlib.a

cd $root_dir


ls -l ../server/server_frame/lib

cd $root_dir
