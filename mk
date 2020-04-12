#!/bin/bash

rm proj diskimage 
sudo ./mkdisk2
gcc main.c -o proj 
./proj
