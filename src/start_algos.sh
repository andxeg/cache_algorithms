#!/bin/sh

maxCacheSize=$1
step=$2
dataFile=$3

algdir="./build/alg"

# Cache replacement algorithms
algs="arc fifo lfu lru mid mq s4lru 2q"
for alg in $algs
do
    for size in $(seq $step $step $maxCacheSize)
    do
        $algdir/cachealg $alg $size "$dataFile" >> $alg.txt
    done
done

# Optimal algorithm

optalgdir="./build/opt"
for size in $(seq $step $step $maxCacheSize)
do
    $optalgdir/opt $size "$dataFile" >> "opt.txt"
done

