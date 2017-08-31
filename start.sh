#!/bin/bash


ALGO_PATH=$1
CACHE_POLICY=$2
SIZES="1000000000 4493148160 6740246528 8987344896 13481541632 22469935104"
DATASET_PATH="/home/andrew/MEGA/ARCCN_CDN/second_stage/user_log/analyze_user_log_backup_22_08_2017/"$3


if [ $# -eq 0 ]
then
	echo "Some arguments were missed."
	echo "Example: ./start.sh ./build/alg/cachealg lru dataset2/cid_size/pid_5.csv"
	exit
fi


for size in $SIZES
do
	echo $ALGO_PATH $CACHE_POLICY $size $DATASET_PATH
	$ALGO_PATH $CACHE_POLICY $size $DATASET_PATH
done

# for results
# type command
# ./start.sh ./build/alg/cachealg lru dataset2/cid_size/pid_5.csv > out.txt ; cat out.txt | grep Hit
