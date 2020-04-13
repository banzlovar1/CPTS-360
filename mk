#!/bin/bash

rm proj diskimage 
sudo ./mkdisk2
gcc c/main.c -o proj 
./proj
