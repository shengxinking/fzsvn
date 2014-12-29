#!/usr/bin/env bash 

PATH=$PATH:/opt/mariadb/bin/

# mysql table
mysql -p -e "create database if not exists zgtest"
mysql -p -D zgtest -e "create table if not exists test (fileid int(10) not null, offset int(10) not null,date int(10) not null, time int(10) not null, key fileid_index (fileid));"

# sqlite table
create table test (fileid integer not null, offset integer, date integer, time integer);
create index fileid_index on test(fileid);


