#!/bin/sh
# mysql_backup.sh: backup mysql databases and keep newest 5 days backup.
# db_user is mysql username
# db_passwd is mysql password
# db_host is mysql host
db_user="root"
db_passwd="mengtoy400"
db_host="localhost"
# the directory for story your backup file.
backup_dir="/data/sqldump"
# date format for backup file (dd-mm-yyyy)
time="$(date +"%d-%m-%Y")"
# mysql, mysqldump and some other bin's path
MYSQL="/usr/bin/mysql"
#MYSQL="mysql"
MYSQLDUMP="/usr/bin/mysqldump"
MKDIR="/bin/mkdir"
RM="/bin/rm"
MV="/bin/mv"
GZIP="/bin/gzip"
# check the directory for store backup is writeable  
test ! -w $backup_dir && echo "Error: $backup_dir is un-writeable." && exit 0
# the directory for story the newest backup
test ! -d "$backup_dir/backup.0/" && $MKDIR "$backup_dir/backup.0/"
# get all databases
all_db=("db_account" "db_server" "game_pay")
tLen=${#all_db[@]}
for ((i=0;i<${tLen};i++))
do
db=${all_db[$i]}
$MYSQLDUMP --add-drop-table -u $db_user -h $db_host -p$db_passwd $db | $GZIP -9 > "$backup_dir/backup.0/$time.$db.gz"
done
# delete the oldest backup
test -d "$backup_dir/backup.5/" && $RM -rf "$backup_dir/backup.5"
# rotate backup directory
for int in 4 3 2 1 0
do
if(test -d "$backup_dir"/backup."$int")
then
next_int=`expr $int + 1`
$MV "$backup_dir"/backup."$int" "$backup_dir"/backup."$next_int"
fi
done
exit 0;
