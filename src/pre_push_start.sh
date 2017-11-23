#!/bin/bash


ALGO_PATH=$1
CACHE_POLICY=$2
SIZES="1000000000 4493148160 6740246528 8987344896 13481541632 22469935104"
DATASET_PATH="/home/andrew/MEGA/ARCCN_CDN/second_stage/user_log/analyze_user_log_backup_22_08_2017/"$3
LEARN_PERIOD=1000
RE_ESTIMATION_PERIOD=10000
CONFIG_PATH="/home/andrew/MEGA/ARCCN_CDN/second_stage/code/cache_algorithms/src/config.cfg"

if [ $# -eq 0 ]
then
	echo "Some arguments were missed."
	echo "Example: ./start.sh ./build/alg/cachealg lru dataset2/cid_size/pid_5.csv"
	exit
fi

declare -a pids


# PRE_PUSH_HISTORY_WINDOW
WINDOW_MIN=1
WINDOW_MAX=5
WINDOW_STEP=1

# START_PRE_PUSH
START_MIN=1
START_MAX=5
START_STEP=1

# PRE_PUSH_HISTORY_HOT_CONTENT
HIST_HC_MIN=0.01
HIST_HC_MAX0=0.2
HIST_HC_STEP=0.01

# CACHE_HOT_CONTENT
CACHE_HC_MIN=0.01
CACHE_HC_MAX=0.2
CACHE_HC_STEP=0.01


for 
do
	i=0
	for size in $SIZES
	do
		echo $ALGO_PATH $CACHE_POLICY $size $LEARN_PERIOD $RE_ESTIMATION_PERIOD $DATASET_PATH $CONFIG_PATH
		$ALGO_PATH $CACHE_POLICY $size $LEARN_PERIOD $RE_ESTIMATION_PERIOD $DATASET_PATH $CONFIG_PATH > ./build/$size".txt" &
		pids[${i}]=$!
		let "i+=1"
	done

	echo "waiting"
	for pid in ${pids[*]}
	do
		wait $pid
	done
done

# for results
# type command
# ./start.sh .//build/alg/cachealg s4lru dataset2/pids_without_exceptional_contents/dataset2_pid_5_time_cid_size_without_except_content.log
