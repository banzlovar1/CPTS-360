#!/bin/bash

rm proj mydisk
./mkdisk
gcc main.c -o proj 
./proj $1
