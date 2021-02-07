#!/bin/bash
#Этот скрипт создает основной бэкап, его нужно сделать один раз т.к потом будет работать скрипт
#который создает и накатывает инкрементные бэкапы на основной бэкап
db_backup=/home/user/main_back
# Удаляем данные в каталоге бекапа
rm -rf $db_backup*

# Cоздаём бекап
xtrabackup --user=xtrabackup --password=e81kOcHXOBKx --backup --target-dir=$db_backup

# Выполняем подготовку бекапа для развёртывания
xtrabackup --prepare --target-dir=$db_backup
# Восстановление основного бэкапа
#/etc/init.d/mysql stop
#rm -rf /var/lib/mysql/*
#xtrabackup --copy-back --target-dir=$db_backup
#chown -R mysql:mysql /var/lib/mysql
