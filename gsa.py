import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) > 1:
    path = sys.argv[1]

    input_file = open(path, "r")
    df = pd.read_csv(input_file)
    key_param = "evacuation_time"

    params = df.drop(key_param, axis = 1).columns
    length = df.shape[0]
    print("%d data points" %length)
    
    random_set_count = length
    run_count = 1
    #random_set_count = length // 2
    #run_count = 100
    bin_count = 100
    assert random_set_count % bin_count == 0
    sobol_indices = np.empty((run_count, len(params)))
    for i in range(run_count):
        tmp_df = df.sample(n = random_set_count).copy()
        y_var = np.var(tmp_df[key_param], ddof = 1)
        for j in range(len(params)):
            param = params[j]
            values = tmp_df[[param, key_param]].sort_values(param)
            samples_per_bin = int(random_set_count / bin_count)
            bin_values = np.empty(bin_count)
            for k in range(bin_count):
                offset = k * samples_per_bin
                bin_y = values[key_param].iloc[offset : offset + samples_per_bin]
                bin_values[k] = bin_y.mean()
            sensitivity = np.var(bin_values, ddof = 1)
            main_effect_index = sensitivity / y_var
            sobol_indices[i, j] = main_effect_index
    sobol_means = np.mean(sobol_indices, axis = 0)
    sobol_stdevs = np.std(sobol_indices, axis = 0, ddof = 1) if run_count > 1 else np.zeros_like(sobol_means)
    confidence = 1.96 * sobol_stdevs / np.sqrt(run_count)
    sum = 0
    for param, index, conf in zip(params, sobol_means, confidence):
        sum += index
        print("%30s : %7.5f %f" %(param, index, conf))
    print("sum : %f" %sum)
else:
    print("Usage: %s <path_to_the_file>" %sys.argv[0])
