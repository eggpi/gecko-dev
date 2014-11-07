import os
import sys
import shutil
import matplotlib.pyplot as plot

GRAPH_DIR = 'graphs'
if os.path.isdir(GRAPH_DIR):
    shutil.rmtree(GRAPH_DIR)
os.mkdir(GRAPH_DIR)

data = {}
for allocator in sys.argv[1:]:
    with open(allocator) as measurementsf:
        for line in measurementsf:
            fields = line.split(';')
            fields[0] = ''.join(fields[0].split()[1:]) # ignore mOps

            for f in fields:
                metric, amount = map(str.strip, f.split(':'))
                amount = int(amount) / (1024*1024)
                if metric not in data:
                    data[metric] = {}
                data[metric].setdefault(allocator, []).append(amount)

assert len(data['rss']['mozjemalloc']) == len(data['rss']['jemalloc3'])

for metric in data:
    plot.figure()

    plot_diff = len(data[metric]) == 2
    for allocator in data[metric]:
        measurements = data[metric][allocator]

        if plot_diff:
            plot.subplot(211)

        plot.plot(range(len(measurements)), measurements, label = allocator,
                  marker = 'o', linestyle = '-')
        plot.ylabel(metric + ' (MiB)')

        ax = plot.subplot(211)
        box = ax.get_position()
        ax.set_position([box.x0, box.y0 - box.height * 0.05, box.width, box.height])
        plot.legend(loc = 'upper center', bbox_to_anchor = (0.5, 1.4))

    if plot_diff:
        a1, a2 = data[metric].keys()
        diff = [m1 - m0 for m1, m0 in zip(data[metric][a1], data[metric][a2])]
        plot.subplot(212)
        plot.plot(range(len(diff)), diff, marker = 'o', linestyle = '-')
        plot.ylabel(' - '.join([a1, a2]))
        plot.subplots_adjust(hspace = 0.22, top = 0.85)

    plot.savefig(os.path.join(GRAPH_DIR, metric + '.png'))
