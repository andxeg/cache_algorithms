#!/bin/bash


ALGO_PATH=$1
CACHE_POLICY=$2
SIZES="1000000000 4493148160 6740246528 8987344896 13481541632 22469935104"
DATASET_PATH="/home/andrew/MEGA/ARCCN_CDN/second_stage/user_log/analyze_user_log_backup_22_08_2017/"$3
LEARN_PERIOD=1000
RE_ESTIMATION_PERIOD=10000
CONFIG_FILE="/home/andrew/MEGA/ARCCN_CDN/second_stage/code/cache_algorithms/src/config.cfg"
RESULT_CATALOG="/home/andrew/MEGA/ARCCN_CDN/second_stage/code/cache_algorithms/src/build"

if [ $# -eq 0 ]
then
	echo "Some arguments were missed."
	echo "Example: ./start.sh ./build/alg/cachealg lru dataset2/cid_size/pid_5.csv"
	exit
fi

declare -a pids


# PRE_PUSH_HISTORY_WINDOW
WINDOW_MIN=1
WINDOW_MAX=1
WINDOW_STEP=1

# START_PRE_PUSH
START_MIN=1
START_MAX=1
START_STEP=1

# PRE_PUSH_HISTORY_HOT_CONTENT
HIST_HC_MIN=0.05
HIST_HC_MAX=0.05
HIST_HC_STEP=0.05

# CACHE_HOT_CONTENT
CACHE_HC_MIN=0.05
CACHE_HC_MAX=0.05
CACHE_HC_STEP=0.05

# SIZE_FILTER_ENABLE
SIZE_FILTER_MIN=1
SIZE_FILTER_MAX=1
SIZE_FILTER_STEP=1

# SIZE_FILTER_WINDOW
SIZE_FILTER_WINDOW_MIN=1
SIZE_FILTER_WINDOW_MAX=1
SIZE_FILTER_WINDOW_STEP=1

# SIZE_FILTER_TYPE
SIZE_FILTER_TYPE_MIN=1
SIZE_FILTER_TYPE_MAX=1
SIZE_FILTER_TYPE_STEP=1

# SIZE_FILTER_REVERSED_SIZE
REVERSED_MIN=1
REVERSED_MAX=1
REVERSED_STEP=1

# for dots in float in seq instead commas
LANG=en_US

for w in $(seq $WINDOW_MIN $WINDOW_STEP $WINDOW_MAX)
do
	for s1 in $(seq $START_MIN $START_STEP $START_MAX)
	do
		for h1 in $(seq $HIST_HC_MIN $HIST_HC_STEP $HIST_HC_MAX)
		do
			for h2 in $(seq $CACHE_HC_MIN $CACHE_HC_STEP $CACHE_HC_MAX)
			do
				for f1 in $(seq $SIZE_FILTER_MIN $SIZE_FILTER_STEP $SIZE_FILTER_MAX)
				do
					for f2 in $(seq $SIZE_FILTER_WINDOW_MIN $SIZE_FILTER_WINDOW_STEP $SIZE_FILTER_WINDOW_MAX)
					do
						for f3 in $(seq $SIZE_FILTER_TYPE_MIN $SIZE_FILTER_TYPE_STEP $SIZE_FILTER_TYPE_MAX)
						do
							for f4 in $(seq $REVERSED_MIN $REVERSED_STEP $REVERSED_MAX)
							do
								# First. Modify config file
								sed -i "s/^\(PRE_PUSH_HISTORY_WINDOW\s*=\s*\).*\$/\1$w/" $CONFIG_FILE
								sed -i "s/^\(START_PRE_PUSH\s*=\s*\).*\$/\1$s1/" $CONFIG_FILE
								sed -i "s/^\(PRE_PUSH_HISTORY_HOT_CONTENT\s*=\s*\).*\$/\1$h1/" $CONFIG_FILE
								sed -i "s/^\(CACHE_HOT_CONTENT\s*=\s*\).*\$/\1$h2/" $CONFIG_FILE
								sed -i "s/^\(SIZE_FILTER_ENABLE\s*=\s*\).*\$/\1$f1/" $CONFIG_FILE
								sed -i "s/^\(SIZE_FILTER_WINDOW\s*=\s*\).*\$/\1$f2/" $CONFIG_FILE
								sed -i "s/^\(SIZE_FILTER_TYPE\s*=\s*\).*\$/\1$f3/" $CONFIG_FILE
								sed -i "s/^\(SIZE_FILTER_REVERSED_SIZE\s*=\s*\).*\$/\1$f4/" $CONFIG_FILE

								# Second. Launch algo with different sizes
								i=0
								for size in $SIZES
								do
									echo $ALGO_PATH $CACHE_POLICY $size $LEARN_PERIOD $RE_ESTIMATION_PERIOD $DATASET_PATH $CONFIG_FILE $RESULT_CATALOG/$size"_"$w"_"$s1"_"$h1"_"$h2"_"$f1"_"$f2"_"$f3"_"$f4".txt"									
									$ALGO_PATH $CACHE_POLICY $size $LEARN_PERIOD $RE_ESTIMATION_PERIOD $DATASET_PATH $CONFIG_FILE > $RESULT_CATALOG/$size"_"$w"_"$s1"_"$h1"_"$h2"_"$f1"_"$f2"_"$f3"_"$f4".txt" &
									pids[${i}]=$!
									let "i+=1"
								done

								# Third. Waiting until algos will be finished.
								echo "waiting"
								for pid in ${pids[*]}
								do
									wait $pid
								done
							done
						done
					done
				done
			done
		done
	done
done




# for results
# type command
# ./pre_push_start.sh ./build/alg/cachealg s4lru dataset2/pids_without_exceptional_contents/dataset2_pid_5_time_cid_size_without_except_content.log
# ./build/alg/cachealg s4lru 1000000000 1000 10000 /home/andrew/ARCCN_CDN/analyze_user_log_backup_22_08_2017/datasets_without_exceptional_contents/dataset2.log config.cfg