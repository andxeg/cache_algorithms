#!/bin/sh

maxCacheSize=$1
step=$2
dataFile=$3

algs="arc fifo lfu lru mid mq s4lru 2q"
for alg in $algs
do
    for size in $(seq $step $step $maxCacheSize)
    do
        ./cachealg $alg $size "$dataFile"
    done
done
