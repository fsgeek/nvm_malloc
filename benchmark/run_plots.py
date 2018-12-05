import argparse
import os
import subprocess

import matplotlib as mpl
mpl.use("Agg")
from pylab import *
#from run_benchmarks import * as rb

# make plots look better
#plt.rcParams['text.latex.preamble']=[r"\usepackage{lmodern}"]
plt.rcParams['text.latex.preamble']=[r"\usepackage{lmodern}", r"\usepackage[T1]{fontenc}"]
params = {'text.usetex' : True,
          'font.size' : 36,
          'font.weight': 'bold',
          'font.family' : 'sans-serif',
          'text.latex.unicode': True,
          }
plt.rcParams.update(params)

JEMALLOC_PATH = "/usr/local/lib/libjemalloc.so"
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
    if marker_index >= len(markers): marker_index = 0
    mi = marker_index
    marker_index += 1
    return markers[mi]

color_index = 0
def getNextColor():
    global color_index
    colors = ['black', 'green', 'blue', 'red', 'purple', 'orange', 'yellow']
    if color_index >= len(colors): color_index = 0
    ci = color_index
    color_index += 1
    return colors[ci]

line_style_index = 0
def getNextLineStyle():
    global line_style_index
    line_styles = ['-', '--', '-.', ':']
    if line_style_index >= len(line_styles): line_style_index = 0
    lsi = line_style_index
    line_style_index += 1
    return line_styles[lsi]

def getCacheFileName(binary, args, with_jemalloc):
    if 'cache_dir' in args: wd = args['cache_dir']
    else: wd = os.getcwd()
    return os.path.join(wd, "cached", "%s_%d_%d_%d_%d_%s" % (binary,
                                                                      args["threads_min"],
                                                                      args["threads_max"],
                                                                      args["payload_min"],
                                                                      args["payload_max"],
                                                                      "true" if with_jemalloc else "false"))


def runBenchmark(binary, args, with_jemalloc=False):
    cachefile = getCacheFileName(binary, args, with_jemalloc)
    print('opening cachefile {}'.format(cachefile))
    assert(os.path.isfile(cachefile))
    return eval(open(cachefile).read())

def plotBenchmark(benchname, args):
    global marker_index
    marker_index = 0 # reset marker index
    global color_index
    color_index = 0
    global line_style_index
    line_style_index = 0
    fig = plt.figure()
    fig.set_size_inches(args['plot_width'], args['plot_height'])
    plt.title(BENCHTITLES[benchname])
    plt.ylabel("Time in $ms$")
    plt.xlabel("Parallel Threads")
    plt.xlim(1, args["threads_max"])
    plotX = arange(1, args["threads_max"]+1)

    # default allocator
    #print "Running '%s' for default malloc" % benchname
    #plt.plot(plotX, runBenchmark("bench_%s" % benchname, args), label="default malloc", ls="--", color="black")

    # if selected, run with jemalloc
    if args["with_jemalloc"]:
        print "Running '%s' for jemalloc" % benchname
        plt.plot(plotX, runBenchmark("bench_%s" % benchname, args, True), label="jemalloc (DRAM)", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # run standard nvm_malloc
    if not args["has_clflushopt"] and not args["has_clwb"]:
        print "Running '%s' for nvm_malloc" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm" % benchname, args), label="nvm\_malloc", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run nvm_malloc with CLFLUSHOPT
    if args["has_clflushopt"] and not args["has_clwb"]:
        print "Running '%s' for nvm_malloc with CLFLUSHOPT" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm_clflushopt" % benchname, args), label="nvm\_malloc with CLFLUSHOPT", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run nvm_malloc with CLWB
    if args["has_clwb"]:
        print "Running '%s' for nvm_malloc with CLWB" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm_clwb" % benchname, args), label="nvm\_malloc with CLWB", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run nvm_malloc with fences disables
    if args["with_nofence"]:
        print "Running '%s' for nvm_malloc without fences" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm_nofence" % benchname, args), label="nvm\_malloc no fences", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run nvm_malloc with flushes disabled
    if args["with_noflush"]:
        print "Running '%s' for nvm_malloc without flushes" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm_noflush" % benchname, args), label="nvm\_malloc no flushes", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run nvm_malloc with both fences and flushes disabled
    if args["with_none"]:
        print "Running '%s' for nvm_malloc without flushes or fences" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_nvm_none" % benchname, args), label="nvm\_malloc no fences/flushes", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run pmdk_malloc
    if args["with_pmdk"]:
        print "Running '%s' for pmcto_malloc" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_pmdk" % benchname, args), label="pmcto\_malloc", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run pmobj_malloc
    if args["with_pmobj"]:
        print "Running '%s' for pmobj_malloc" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_pmobj" % benchname, args), label="pmobj\_malloc", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    # if selected, run makalu_malloc
    if args["with_makalu"]:
        print "Running '%s' for makalu_malloc" % benchname
        plt.plot(plotX, runBenchmark("bench_%s_makalu" % benchname, args), label="makalu\_malloc", 
                 ls=getNextLineStyle(), marker=getNextMarker(), color=getNextColor())

    #plt.legend(loc='upper left', prop={'size':36})
    plt.legend(loc='upper left', prop={'size':36})
    figname = "{}.pdf".format(benchname)
    figname = os.path.join(os.getcwd(), "plots", "{}.pdf".format(benchname))
    croppedname = os.path.join(os.getcwd(), "plots", "{}-crop.pdf".format(benchname))
    plt.savefig(figname, dpi=1000, bbox_inches="tight")
    plt.close()
    if 0 == subprocess.call(['pdfcrop', figname]): os.rename(croppedname, figname)

def plotRecoveryBenchmark(args):
    return

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
    parser.add_argument("--has-clflushopt", action="store_true", default=True)
    parser.add_argument("--has-clwb", action="store_true", default=True)
    parser.add_argument("--with-jemalloc", action="store_true", help="include a run with jemalloc in the benchmark", default=True)
    parser.add_argument("--with-nofence", action="store_true", help="include a run with disabled fences", default=False)
    parser.add_argument("--with-noflush", action="store_true", help="include a run with disabled flushes", default=False)
    parser.add_argument("--with-none", action="store_true", help="include a run with disabled fences and flushes",default=True)
    parser.add_argument("--ignore-cached", action="store_true")
    parser.add_argument("--with-pmdk", action="store_true", default=True)
    parser.add_argument("--with-pmobj", action="store_true", default=True)
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

    # make sure the cache and plot folders exists
    assert('cache_dir' in args)
    assert('plot_dir' in args)
    assert(os.path.isdir(args['cache_dir']))
    assert(os.path.isdir(args['plot_dir']))

    # plot regular benchmarks
    for benchmark in BENCHMARKS:
        if args["run_%s" % benchmark] or args["run_all"]:
            print("plotbenchmark".format(benchmark))
            plotBenchmark(benchmark, args)
