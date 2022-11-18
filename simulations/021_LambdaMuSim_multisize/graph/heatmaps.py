#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np


def readFromFile(filename: str, scale: float):
    data_map = dict()
    with open(filename, "r") as infile:
        for line in infile:
            (x, y, value, err) = line.rstrip().split(" ")
            if x not in data_map:
                data_map[x] = dict()
            data_map[x][y] = float(value) * scale
            # print(f"{x},{y} = {value}")

    xtics = []
    ytics = []
    data = []
    for x, m in sorted(data_map.items(), key=lambda x: float(x[0])):
        xtics.append(x)
        cur_data = []
        for y, v in sorted(m.items(), key=lambda x: float(x[0])):
            if y not in ytics:
                ytics.append(y)
            cur_data.append(v)
        data.append(cur_data)
    return (xtics, ytics, data)


def saveHeatMap(dir: str, metric: str, numapps: int, algo: str, norm: bool, title: str):
    mangle = metric + "-" + str(numapps) + "-" + str(algo)
    print(mangle)
    scale = 1.0 / numapps if norm else 1.0
    (xtics, ytics, data) = readFromFile(dir + mangle + ".dat", scale)

    fig, ax = plt.subplots(figsize=(5, 5))
    im = ax.imshow(data, cmap="coolwarm", interpolation="bilinear")
    cbar = ax.figure.colorbar(im, ax=ax, shrink=0.75)
    ax.set_xticks(np.arange(len(xtics)))
    ax.set_xticklabels(xtics)
    ax.set_yticks(np.arange(len(ytics)))
    ax.set_yticklabels(ytics)
    ax.set_title(title)
    ax.set_xlabel(r"$d_2$")
    ax.set_ylabel(r"$m_2$")
    fig.tight_layout()
    # plt.show()
    plt.savefig(mangle + ".pdf", format="pdf", dpi=150)
    plt.close()


if __name__ == "__main__":
    metrics = {
        "lambda-cost": [True, r"$Cost/\lambda$-app"],
        "lambda-service-cloud": [False, r"Ratio of $\lambda$-apps in the cloud "],
    }
    numapps = [50, 100, 150, 200]
    algos = ["random", "greedy", "proposed"]
    metrics_norm = {"lambda-cost", "mu-cost"}
    dir = "../post/"

    for m, params in metrics.items():
        for n in numapps:
            for a in algos:
                saveHeatMap(dir, m, n, a, params[0], params[1])
