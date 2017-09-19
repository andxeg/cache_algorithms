#!/bin/bash

ALGO_PATH="/home/soc/ARCCN/algorithms/cache/build/alg/cachealg"
DATASET_PATH="/home/soc/ARCCN/dataset2_pid_5_time_cid_size.log"
SIZES="1000000000 4493148160 6740246528 8987344896 13481541632 22469935104"

MIN_LEARN_LIMIT=40000
MAX_LEARN_LIMIT=160000
LEARN_LIMIT_STEP=40000
RE_ESTIMATION_PERIODS="10000"

RE_ESTIMATION_PERIODS_MIN="2000"
RE_ESTIMATION_PERIODS_MAX="20000"
RE_ESTIMATION_PERIODS_STEP="2000"

declare -a pids


for period in $(seq $RE_ESTIMATION_PERIODS_MIN $RE_ESTIMATION_PERIODS_STEP $RE_ESTIMATION_PERIODS_MAX)
do
	for learn_limit in $(seq $MIN_LEARN_LIMIT $LEARN_LIMIT_STEP $MAX_LEARN_LIMIT)
	do
<<<<<<< Updated upstream
		# mkdir "/home/soc/ARCCN/results_pop_caching/l_"$learn_limit"_re_"$period
=======
		mkdir "/home/soc/ARCCN/results_pop_caching/l_"$learn_limit"_re_"$period
>>>>>>> Stashed changes
		DIR="/home/soc/ARCCN/results_pop_caching/l_"$learn_limit"_re_"$period
		i=0
		for size in $SIZES
		do
			echo "$ALGO_PATH pop_caching $size $learn_limit $period $DATASET_PATH > $DIR/$size".txt""
			$ALGO_PATH pop_caching $size $learn_limit $period $DATASET_PATH > $DIR/$size".txt" &
			pids[${i}]=$!
			let "i+=1"
		done

		echo "waiting"
		for pid in ${pids[*]}
		do
			wait $pid
		done

	done
done
