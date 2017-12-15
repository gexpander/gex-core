#!/bin/bash

echo "-- Building parser from Ragel source --"

ragel -L -G0 ini_parser.rl -o ini_parser.c
