#!/bin/bash
nvim main.c
tcc main.c -ltermbox -g
./a.out
