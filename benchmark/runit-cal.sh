#!/bin/sh
export VMMALLOC_POOL_SIZE=$((64*1024*1024*1024))
export VMMALLOC_POOL_DIR="/mnt/pmem7/fsgeek"
sizes = (256 512 768 1024 1280 1536 1792 2048 2304 2560 2816 3072 3328 3584 3840 4096)
for i in sizes:
    mkdir "size-"$i
exit 0
mkdir size-256 size-512 size-768 size-1024 size-1280 size-1536 size-1792 size-2048 size-2304 size-2560 size-2816 size-3072 size-3328 size-3584 size-3840 size-4096
mkdir cached plots
python run_benchmarks.py --payload-min 64 --payload-max 64 --run-all --node 1 --verbose
mv cached plots size-64
mkdir cached plots
python run_benchmarks.py --payload-min 256 --payload-max 256 --run-all --node 1 --verbose
mv cached plots size-256
mkdir cached plots
python run_benchmarks.py --payload-min 1024 --payload-max 1024 --run-all --node 1 --verbose
mv cached plots size-1024
mkdir cached plots
#python run_benchmarks.py --payload-min 256 --payload-max 256 --run-all --node 1 --verbose
