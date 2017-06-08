import os
import subprocess

import runner
import parsers
import printers

PROJECT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../")


def call(cmd, cwd):
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    try:
        out, err = proc.communicate(timeout=5*60)
    except subprocess.TimeoutExpired:
        proc.kill()
        raise
    print(out)
    return proc.returncode, out.decode("utf-8")


if __name__ == '__main__':

    print(os.getcwd())

    builders = {
        "BDW GC": runner.CMakeBuilder(PROJECT_DIR, "BDW_GC", build_type="Release"),
        "gc_ptr_serial": runner.CMakeBuilder(PROJECT_DIR, "PRECISE_GC_SERIAL", build_type="Release"),
        "gc_ptr_cms": runner.CMakeBuilder(PROJECT_DIR, "PRECISE_GC_CMS", build_type="Release")
    }

    boehm_cmd = "GC_PRINT_STATS=1 {runnable} {args}"

    suites = {
        "BDWGC": {"builder": builders["BDW GC"], "cmd": boehm_cmd, "parser": parsers.BoehmStatsParser()},
        "BDWGC-incremental": {"builder": builders["BDW GC"], "cmd": boehm_cmd, "args": ["--incremental"], "parser": parsers.BoehmStatsParser()},
        "gc-ptr-serial": {"builder": builders["gc_ptr_serial"], "parser": parsers.GCPauseTimeParser()},
        "gc-ptr-cms": {"builder": builders["gc_ptr_cms"], "parser": parsers.GCPauseTimeParser()}
    }

    targets = {
        "gcbench-top-down": {
            "name": "boehm",
            "runnable": "benchmark/boehm/boehm",
            "suites": ["BDWGC", "BDWGC-incremental", "gc-ptr-serial", "gc-ptr-cms"],
            "params": ["--top-down"]
        }
        # "gcbench bottom-up": {
        #     "name": "boehm",
        #     "runnable": "benchmark/boehm/boehm",
        #     "suites": ["manual", "shared_ptr", "BDW GC", "BDW GC incremental", "gc_ptr serial", "gc_ptr cms"],
        #     "params": ["bottom-up"]
        # },
        # "parallel merge sort": {
        #     "name": "parallel_merge_sort",
        #     "runnable": "benchmark/parallel_merge_sort/parallel_merge_sort",
        #     "suites": ["manual", "shared_ptr", "BDW GC", "BDW GC incremental", "gc_ptr serial", "gc_ptr cms"]
        # },
        # "cord-build": {
        #     "name": "cord",
        #     "runnable": "benchmark/cord/cord",
        #     "suites": ["shared_ptr", "BDW GC", "BDW GC incremental", "gc_ptr serial", "gc_ptr cms"],
        #     "params": ["build", {"len": [6]}]
        # },
        # "cord-substr": {
        #     "name": "cord",
        #     "runnable": "benchmark/cord/cord",
        #     "suites": ["shared_ptr", "BDW GC", "BDW GC incremental", "gc_ptr serial", "gc_ptr cms"],
        #     "params": ["substr", {"len": [6]}]
        # },
        # "cord-flatten": {
        #     "name": "cord",
        #     "runnable": "benchmark/cord/cord",
        #     "suites": ["shared_ptr", "BDW GC", "BDW GC incremental", "gc_ptr serial", "gc_ptr cms"],
        #     "params": ["flatten", {"len": [5]}]
        # }
    }

    printer = printers.GCPauseTimePlotPrinter()

    for name, target in targets.items():
        for suite_name in target["suites"]:
            suite = suites[suite_name]
            build = suite["builder"].build(target["name"])
            parser = suite["parser"]

            args = suite.get("args", []) + target["params"]

            cmd = suite.get("cmd")
            if cmd:
                cmd = cmd.format(runnable=target["runnable"], args=" ".join(args))
            else:
                cmd = "{} {}".format(target["runnable"], " ".join(args))

            rc, out = call(cmd, build.dirname())
            assert rc == 0
            parser.parse(out)
            printer.print_report(parser.result(), "{}-{}".format(name, suite_name))