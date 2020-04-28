#!/bin/bash

if [ -z "$1" ]
then
    echo "[mk]: Must pass a disk name to execute."
    exit -1
fi

cp $1 "$1_cpy"
echo "[mk]: copied $1 to $1_cpy"

gcc main.c -o proj 

echo "[mk]: executing compiled proj with disk=$1_cpy"
./proj "$1_cpy"

echo "[mk]: removing $1_cpy and proj"
rm "$1_cpy" proj
