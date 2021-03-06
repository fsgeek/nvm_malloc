import argparse
import os
import subprocess

import matplotlib as mpl
mpl.use("Agg")
from pylab import *

# global variables
#verbose = False

# make plots look better
plt.rcParams['text.latex.preamble']=[r"\usepackage{lmodern}"]
params = {'text.usetex' : True,
          'font.size' : 24,
          'font.family' : 'lmodern',
          'text.latex.unicode': True,
          }
plt.rcParams.update(params)

#JEMALLOC_PATH = "/usr/local/lib/libjemalloc.so"
JEMALLOC_PATH = "libvmmalloc.so.1"
BENCHMARKS = ["alloc_free", "alloc_free_alloc", "fastalloc", "linkedlist"]
BENCHTITLES = {"alloc_free": "Allocate and Free",
               "alloc_free_alloc": "Allocate, Free and Allocate",
               "fastalloc": "Allocation Loop",
               "linkedlist": "Linked List Creation",
               "recovery": "nvm\_malloc Internal Recovery"}

marker_index = 0
def getNextMarker():
    global marker_index
    markers = ['o', 's', 'P', 'H', '+', 'X', 'D', 10, '|', 'x', 1, 3, 4, 5, 6, 7]
    m = markers[marker_index]
    marker_index += 1
    return m

def getCacheFileName(binary, args, with_jemalloc):
    return os.path.join(os.getcwd(), "cached", "%s_%d_%d_%d_%d_%s" % (binary,
                                                                      args["threads_min"],
                                                                      args["threads_max"],
                                                                      args["payload_min"],
                                                                      args["payload_max"],
                                                                      "true" if with_jemalloc else "false"))

def runBenchmarkBinary(binary, parameters, with_jemalloc, node=0, runs=5, miliseconds=False):
    procString = "hwloc-bind node:{} ".format(node) + os.path.join(os.getcwd(), "build", binary) + " " + " ".join([str(p) for p in parameters])
    env = os.environ.copy()
    if with_jemalloc:
        env['LD_PRELOAD'] = JEMALLOC_PATH
	env['VMMALLOC_POOL_SIZE']=str(64*1024*1024*1024)
        env['VMMALLOC_POOL_DIR']="/mnt/pmem7/fsgeek"
    elapsed = 0.0
    global verbose
    if verbose: print("run {} {} times".format(procString, runs))
    for i in range(runs):
        proc = subprocess.Popen(procString, shell=True, stdout=subprocess.PIPE, env=env)
        data = proc.stdout.read()
        try:
	        elapsed += float(data)
        except ValueError as e:
            print('return value {} data {} from command {} exception {}'.format(proc.returncode, data, procString, e))
    elapsed /= float(runs)
    return elapsed/1000 if miliseconds else elapsed

def runBenchmark(binary, args, with_jemalloc=False):
    cachefile = getCacheFileName(binary, args, with_jemalloc)
    if not args["ignore_cached"] and os.path.isfile(cachefile):
        return eval(open(cachefile).read())
    result = []
    for numThreads in range(args["threads_min"], args["threads_max"]+1):
        result.append(runBenchmarkBinary(binary, [numThreads, args["payload_min"], args["payload_max"]], 
                                         with_jemalloc, node=args["node"], runs=args["number_of_runs"], miliseconds=True))
    open(cachefile, "w").write(str(result))
    return result

def runRecovery(maxIterations, args):
    cachefile = getCacheFileName("bench_recovery", args, False)
    if not args["ignore_cached"] and os.path.isfile(cachefile):
        return eval(open(cachefile).read())
    result = []
    for numIterations in range(1, maxIterations+1):
        result.append(runBenchmarkBinary("bench_recovery", [numIterations, args["payload_min"], args["payload_max"]], False))
    open(cachefile, "w").write(str(result))
    return result

def plotBenchmark(benchname, args):
    global marker_index
    marker_index = 0 # reset marker index
    fig = plt.figure()
    fig.set_size_inches(args['plot_width'], args['plot_height'])
    plt.title(BENCHTITLES[benchname])
    plt.ylabel("Time in $ms$")
    plt.xlabel("Parallel Threads")
    plt.xlim(1, args["threads_max"])
    plotX = arange(1, args["threads_max"]+1)

    # default allocator
    print("Running '{}' for default malloc".format(benchname))
    plt.plot(plotX, runBenchmark("bench_%s" % benchname, args), label="default malloc", ls="--", color="black")

    # if selected, run with jemalloc
    if args["with_jemalloc"]:
        print("Running '{}' for jemalloc".format(benchname))
        plt.plot(plotX, runBenchmark("bench_%s" % benchname, args, True), label="jemalloc", ls=":", color="black")

    # run standard nvm_malloc
    #print("Running '{}' for nvm_malloc".format(benchname))
    #plt.plot(plotX, runBenchmark("bench_%s_nvm" % benchname, args), label="nvm\_malloc", ls="-", marker=getNextMarker(), color="black")

    # if selected, run nvm_malloc with CLFLUSHOPT
    if args["has_clflushopt"]:
        print("Running '{}' for nvm_malloc with CLFLUSHOPT".format(benchname))
        plt.plot(plotX, runBenchmark("bench_%s_nvm_clflushopt" % benchname, args), label="nvm\_malloc with CLFLUSHOPT", ls="-", marker=getNextMarker(), color="black")

    # if selected, run nvm_malloc with CLWB
    if args["has_clwb"]:
        print( "Running '%s' for nvm_malloc with CLWB" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_nvm_clwb" % benchname, args), label="nvm\_malloc with CLWB", ls="-", marker=getNextMarker(), color="black")

    # if selected, run nvm_malloc with fences disables
    if args["with_nofence"]:
        print( "Running '%s' for nvm_malloc without fences" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_nvm_nofence" % benchname, args), label="nvm\_malloc no fences", ls="-", marker=getNextMarker(), color="black")

    # if selected, run nvm_malloc with flushes disabled
    if args["with_noflush"]:
        print( "Running '%s' for nvm_malloc without flushes" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_nvm_noflush" % benchname, args), label="nvm\_malloc no flushes", ls="-", marker=getNextMarker(), color="black")

    # if selected, run nvm_malloc with both fences and flushes disabled
    if args["with_none"]:
        print( "Running '%s' for nvm_malloc without flushes or fences" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_nvm_none" % benchname, args), label="nvm\_malloc no fences/flushes", ls="-", marker=getNextMarker(), color="black")

    # if selected, run pmdk_malloc
    if args["with_pmdk"]:
        print("Running '%s' for pmcto_malloc" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_pmdk" % benchname, args), label="pmcto\_malloc", ls="-", marker=getNextMarker(), color="black")

    # if selected, run pmobj_malloc
    if args["with_pmobj"]:
        print("Running '%s' for pmobj_malloc" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_pmobj" % benchname, args), label="pmobj\_malloc", ls="-", marker="*", color="black")

    # if selected, run makalu_malloc
    if args["with_makalu"]:
        print("Running '%s' for makalu_malloc" % benchname)
        plt.plot(plotX, runBenchmark("bench_%s_makalu" % benchname, args), label="makalu\_malloc", ls="-", marker=getNextMarker(), color="black")

    plt.legend(loc='upper left', prop={'size':24})
    plt.legend(loc='upper left', prop={'size':24})
    plt.savefig(os.path.join(os.getcwd(), "plots", "%s.pdf" % benchname), dpi=1000, bbox_inches="tight")
    plt.close()

def plotRecoveryBenchmark(args):
    fig = plt.figure()
    fig.set_size_inches(5.31, 3.54)
    plt.title("nvm\_malloc internal recovery")
    plt.ylabel("Recovery time in $\mu s$")
    plt.xlabel("Iterations of 10k allocations")
    maxIterations = 20
    plt.xlim(1, maxIterations)
    plotX = arange(1, maxIterations+1)
    print( "Running 'recovery' for nvm_malloc")
    plt.plot(plotX, runRecovery(maxIterations, args), color="black", ls="-")
    #plt.legend(loc='upper left', prop={'size':10})
    plt.savefig(os.path.join(os.getcwd(), "plots", "recovery.pdf"), dpi=1000, bbox_inches="tight")
    plt.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="nvm_malloc benchmarking tool")
    parser.add_argument("--run-all", action="store_true")
    parser.add_argument("--run-alloc-free", action="store_true")
    parser.add_argument("--run-alloc-free-alloc", action="store_true")
    parser.add_argument("--run-fastalloc", action="store_true")
    parser.add_argument("--run-linkedlist", action="store_true")
    parser.add_argument("--run-recovery", action="store_true")
    parser.add_argument("--threads-min", type=int, default=1)
    parser.add_argument("--threads-max", type=int, default=23)
    parser.add_argument("--payload-min", type=int, default=64)
    parser.add_argument("--payload-max", type=int, default=64)
    parser.add_argument("--has-clflushopt", action="store_true", default=False)
    parser.add_argument("--has-clwb", action="store_true", default=False)
    parser.add_argument("--with-jemalloc", action="store_true", help="include a run with jemalloc in the benchmark", default=True)
    parser.add_argument("--with-nofence", action="store_true", help="include a run with disabled fences", default=False)
    parser.add_argument("--with-noflush", action="store_true", help="include a run with disabled flushes", default=False)
    parser.add_argument("--with-none", action="store_true", help="include a run with disabled fences and flushes",default=False)
    parser.add_argument("--ignore-cached", action="store_true")
    parser.add_argument("--with-pmdk", action="store_true", default=False)
    parser.add_argument("--with-pmobj", action="store_true", default=False)
    parser.add_argument("--with-makalu", action="store_true")
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("--node", type=int, default=0)
    parser.add_argument("--number-of-runs", type=int, default=5)
    parser.add_argument("--plot-width", type=float, default=20.0)
    parser.add_argument("--plot-height", type=float, default=13.0)
    parser.add_argument("--cache-dir", type=str, default='cached')
    parser.add_argument("--plot-dir", type=str, default='plots')
    args = vars(parser.parse_args())
    if args["payload_max"] < args["payload_min"]:
        args["payload_max"] = args["payload_min"]

    global verbose
    verbose = args['verbose']
    #if 'verbose' in args:
    #    global verbose 
    #    verbose = args["verbose"]

    # make sure the cache and plot folders exists
    if not os.path.isdir(args["cache_dir"]):
        os.mkdir(args["cache_dir"])
    if not os.path.isdir(args["plot_dir"]):
        os.mkdir(args["plot_dir"])

    # run regular benchmarks
    for benchmark in BENCHMARKS:
        if args["run_%s" % benchmark] or args["run_all"]:
            plotBenchmark(benchmark, args)

    # run recovery benchmark
    if args["run_recovery"] or args["run_all"]:
        # set thread min/max to 0 to ignore variation for cache file
        args["threads_min"] = 0
        args["threads_max"] = 0
        plotRecoveryBenchmark(args)
