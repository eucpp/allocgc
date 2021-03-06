import json
import subprocess


class JSONPrinter:

    def __init__(self):
        pass

    def print_report(self, report, outfn):
        with open(outfn + ".json", "w") as outfile:
            json.dump(report, outfile, indent=4)


def escape_tex(content):
    return content.replace("{", "{{").replace("}", "}}").replace("[[", "{").replace("]]", "}")


class GCTimePlotPrinter:

    def __init__(self):
        with open('gc-time-plot.tex', 'r') as fd:
            self._tpl = escape_tex(fd.read())

    def print_plot(self, targets, suites, report, outfn, k):
        tpl = escape_tex("""
            \\addplot+[
                [[color]], draw=[[color]], pattern color = [[color]], pattern = [[pattern]],
                error bars/.cd, y dir=both,y explicit,
            ]
            coordinates{
                [[coordinates]]
            };
        """)

        colors = ["brown", "blue", "red", "orange", "violet", "green"]
        patterns = ["horizontal lines", "north west lines", "vertical lines", "dots", "crosshatch", "grid"]

        i = k
        content = ""
        for suite in suites:
            coordinates = ""
            coordinate  = "({n}, {mean}) +- (0, {std})\n"

            n = 1
            for target in targets:
                data = report[target][suite]
                # time = data["full_time"]["mean"]
                mean = float(data["full_time"]["mean"])
                std  = float(data["full_time"]["std"])
                coordinates += coordinate.format(n=n, mean=round(mean, 2), std=round(std, 2))
                n += 1

            content += tpl.format(color=colors[i], pattern=patterns[i], coordinates=coordinates)
            i += 1

        xtick = ", ".join(str(i) for i in range(1, len(targets)+1))
        legend = ", ".join(suite.replace("_", "\\_") for suite in suites)
        xticklabels = ", ".join(target.replace(" ", "\\\\") for target in targets)

        outfn_tex = outfn + ".tex"
        outfn_pdf = outfn + ".pdf"
        with open(outfn_tex, "w") as outfd:
            outfd.write(self._tpl.format(content=content, legend=legend, xtick=xtick, xticklabels=xticklabels))

        proc = subprocess.Popen("pdflatex {fn_tex} && pdfcrop {fn_pdf}".format(fn_tex=outfn_tex, fn_pdf=outfn_pdf), shell=True)
        proc.wait()
        assert proc.returncode == 0

    def print_report(self, report, outfn):
        suites1 = ["manual", "shared_ptr", "BoehmGC", "BoehmGC incremental", "gc_ptr serial", "gc_ptr cms"]
        suites2 = ["shared_ptr", "BoehmGC", "BoehmGC incremental", "gc_ptr serial", "gc_ptr cms"]
        targets1 = ["gcbench top-down", "gcbench bottom-up", "parallel-merge-sort"]
        targets2 = ["cord-build", "cord-substr", "cord-flatten"]
        self.print_plot(targets1, suites1, report, outfn + "-1", 0)
        self.print_plot(targets2, suites2, report, outfn + "-2", 1)


class GCPauseTimePlotPrinter:

    def __init__(self):
        with open('gc-pause-plot.tex', 'r') as fd:
            self._tpl = escape_tex(fd.read())

    def print_plot(self, name, data, outfn):
        import matplotlib.pyplot as plt

        labels = [suite for suite, _ in data]
        pauses = [stats["pause"] for _, stats in data]

        plt.title(name)
        plt.ylabel("Pause time (ms)")

        axes = plt.gca()
        # axes.set_xlim([xmin,xmax])
        axes.set_ylim([0, 15])

        plt.boxplot(pauses, sym='o', labels=labels)
        plt.savefig(outfn)
        plt.clf()

    def print_report(self, data, outfn):
        for name, target in data.items():
            self.print_plot(name, target, "{}-{}.eps".format(outfn, name))


class GCHeapPlotPrinter:

    def __init__(self):
        with open('gc-heap-plot.tex', 'r') as fd:
            self._tpl = escape_tex(fd.read())

    def print_plot(self, data, title, outfn):
        def to_Mb(sz):
            return round(float(sz) / (1024 * 1024), 3)

        content = ""
        time = data["gctime"]
        used = [sz + extra for sz, extra in zip(data["heapsize"], data["heapextra"])]
        used = "\n".join("({}, {})".format(i, to_Mb(sz)) for i, sz in zip(time, used))
        content += "\\addplot[color=pink, fill] coordinates {{\n {} \n}} \closedcycle;".format(used)

        # if data["heapextra"]:
        #     all = data["heapextra"]
        #     all = "\n".join("({}, {})".format(i, to_Mb(sz)) for i, sz in zip(time, all))
        #     content += "\\addplot[color=red] coordinates {{\n {} \n}} \closedcycle;".format(all)

        outfn_tex = outfn + ".tex"
        outfn_pdf = outfn + ".pdf"
        with open(outfn_tex, "w") as outfd:
            outfd.write(self._tpl.format(content=content, title=title))

        proc = subprocess.Popen("pdflatex {fn_tex} && pdfcrop {fn_pdf}".format(fn_tex=outfn_tex, fn_pdf=outfn_pdf), shell=True)
        proc.wait()
        assert proc.returncode == 0

    def print_report(self, report, name):
        for suite, data in report.items():
            self.print_plot(data, suite, "{}-{}".format(name, suite.replace(' ', '-')))
