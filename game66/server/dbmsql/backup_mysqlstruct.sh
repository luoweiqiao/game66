#!/bin/sh
# 保存数据库结构

mysqldump -uroot -p123456 -A -d > ./all.sql
mysqldump -uroot -p123456 chess_sysdata > ./chess_sysdata.sql

