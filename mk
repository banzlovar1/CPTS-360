#!/bin/bash

rm proj testdisk1
cp testdisk testdisk1
gcc c/main.c -o proj 
./proj
