#!/bin/bash

rm proj mydisk
sudo ./mkdisk2
gcc main.c -o proj 
./proj
