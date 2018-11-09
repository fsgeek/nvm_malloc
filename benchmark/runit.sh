#!/bin/sh
mkdir size-64 size-256 size-1024
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
