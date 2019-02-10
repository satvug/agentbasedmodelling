import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) > 1:
    path = sys.argv[1]
    path_unix = path.split("/")
    path_dos = path.split("\\")
    file_name = path
    if len(path_unix) != len(path_dos):
        if len(path_unix) > len(path_dos):
            file_name = path_unix[-1]
        else:
            file_name = path_dos[-1]

    input_file = open(path, "r")
    df = pd.read_csv(input_file)
    key_param = "evacuation_time"
    param = "agent_count"
    if len(sys.argv) > 2:
        param = sys.argv[1]
    else:
        for column in df.columns:
            if column in path:
                param = column
                break
    df = df[[param, key_param]]

    groups = df.groupby(param)
    counts = groups.count()
    stdevs = groups.std(ddof = 1)
    df = groups.mean()
    df[param] = df.index
    df["lower_bound"] = df[key_param] - 1.96 * stdevs[key_param] / np.sqrt(counts[key_param])
    df["upper_bound"] = df[key_param] + 1.96 * stdevs[key_param] / np.sqrt(counts[key_param])

    plt.fill_between(df[param], df["lower_bound"], df["upper_bound"], alpha = 0.4)
    plt.plot(df[param], df[key_param])
    param_name = param.replace("_", " ")
    plt.title("OFAT SA of parameter '%s'" %param_name, fontsize = 14)
    plt.xlabel(param_name, fontsize = 12)
    plt.ylabel(key_param.replace("_", " "), fontsize = 12)
    plt.savefig("figures//" + file_name.split(".")[0])
    #plt.show()
else:
    print("Usage: %s <path_to_the_file> <param>" %sys.argv[0])
