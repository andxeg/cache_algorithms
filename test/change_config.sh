#!/bin/sh

KEY=$1
VAL=$2
FILE=$3

sed -i "s/^\($KEY\s*=\s*\).*\$/\1$VAL/" $FILE
