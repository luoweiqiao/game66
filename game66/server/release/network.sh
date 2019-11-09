#!/bin/bash

if [ -n "$1" ]; then
eth_name=$1
else
eth_name="eth0"
fi

i=0

send_o=`ifconfig $eth_name | grep bytes | awk '{print $6}' | awk -F : '{print $2}'`
recv_o=`ifconfig $eth_name | grep bytes | awk '{print $2}' | awk -F : '{print $2}'`
send_n=$send_o
recv_n=$recv_o

while [ $i -le 100000 ]; do
send_l=$send_n
recv_l=$recv_n
sleep 1
send_n=`ifconfig $eth_name | grep bytes | awk '{print $6}' | awk -F : '{print $2}'`
recv_n=`ifconfig $eth_name | grep bytes | awk '{print $2}' | awk -F : '{print $2}'`
i=`expr $i + 1`
send_r=`expr $send_n - $send_l`
recv_r=`expr $recv_n - $recv_l`
total_r=`expr $send_r + $recv_r`
send_ra=`expr /( $send_n - $send_o /) / $i`
recv_ra=`expr /( $recv_n - $recv_o /) / $i`
total_ra=`expr $send_ra + $recv_ra`
sendn=`ifconfig $eth_name | grep bytes | awk -F /( '{print $3}' | awk -F /) '{print $1}'`
recvn=`ifconfig $eth_name | grep bytes | awk -F /( '{print $2}' | awk -F /) '{print $1}'`
clear
echo "Last second : Send rate: $send_r Bytes/sec Recv rate: $recv_r Bytes/sec Total rate: $total_r Bytes/sec"
echo "Average value: Send rate: $send_ra Bytes/sec Recv rate: $recv_ra Bytes/sec Total rate: $total_ra Bytes/sec"
echo "Total traffic after startup: Send traffic: $sendn Recv traffic: $recvn"
done


