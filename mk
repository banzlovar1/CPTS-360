#!/bin/bash

rm proj mydisk
sudo ./mkdisk
gcc main.c -o proj 
./proj
