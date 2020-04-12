#!/bin/bash

rm proj diskimage 
sudo ./mkdisk
gcc main.c -o proj 
./proj
