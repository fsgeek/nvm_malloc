#!/bin/bash
export VMMALLOC_POOL_SIZE=$((64*1024*1024*1024))
export VMMALLOC_POOL_DIR="/mnt/pmem7/fsgeek"

if [ ! -d cal ]
    then
        mkdir cal
fi
sizes=( 256 512 768 1024 1280 1536 1792 2048 2304 2560 2816 3072 3328 3584 3840 4096 )
for i in "${sizes[@]}"
do
   if [ ! -d "cal/size-"$i ]
   then
      mkdir "size-"$i
      mkdir cached
      mkdir plots
      python run_benchmarks.py --payload-min $i --payload-max $i --run-alloc-free-alloc --node 1 --verbose
      mv cached plots "size-"$i
      mv "size-"$i cal
    # do whatever on $i
   fi
done

