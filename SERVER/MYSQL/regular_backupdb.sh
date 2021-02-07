#!/bin/bash
#Этот скрипт создаем инкрементный бэкап на основе основного и накатывает его на основной бэкап
#Получается что скрипт отслеживает изменения в базе данных и дописывает их к основному бэкапу
#Скрипт предназначен выполняться автоматически, например каждый день
db_backup=/home/user/main_back
db_inc=/home/user/inc_back

#удаляем старый инкрементный бэкап
rm -rf $db_inc*
# Создаем новый инкрементный бэкап
xtrabackup --backup --target-dir=$db_inc --incremental-basedir=$db_backup
# Подготавливаем основной бэкап к накатыванию на него инкремента
xtrabackup --prepare --apply-log-only --target-dir=$db_backup
# Накатываем инкремент на основной бэкап
xtrabackup --prepare --apply-log-only --target-dir=$db_backup --incremental-dir=$db_inc

