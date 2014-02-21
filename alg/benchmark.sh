#!/bin/sh

dataFile=$1
maxCacheSize=$2
step=$3

algs="mid lru lfu 2q s4lru fifo"
for alg in $algs
do
    echo $alg
    for size in $(jot $maxCacheSize $step $maxCacheSize $step)
    do
        ./cachealg $alg $size $dataFile
    done
done
