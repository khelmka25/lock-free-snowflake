import matplotlib.pyplot as plt;
import numpy as np;
import pandas as pd;

# purpose: extract data from datafiles for the testing of different algorithms at different thread counts
# each algorithm has 1 run at thread counts of range [1, 16]

# extract each, we want one plot showing each algorithm's run performance for a given thread count
# thread count of x-axis
# id rate on y-axis (ids/ms)

algorithms = np.array(["v2a", "v2b", "v3a", "v3b", "v3c", "v3d", "v4b", "v4c", "v4d"])
line_types = np.array([".", ".", "-", "-", "-", "-", ".-", ".-", ".-"])
thread_counts = np.array([i for i in range(1, 17)])

# matrix height of the algorithm count
# width the thread count
# np.zeros (rows, cols)
averages = np.zeros((len(algorithms), len(thread_counts)));

for i, thread_count in enumerate(thread_counts):
  for j, algorithm in enumerate(algorithms):
    filename = f"out/lockfree::{algorithm}::get-t{thread_count}.txt"
    with open(filename, "r") as file:
      line = file.readline()

    # each file contains the following data points: average ids/ms
    id_rate = float(line)
  
    # add the average in the corresponding slot
    averages[j][i] = id_rate

print(averages)
# np.savetxt("out.txt", averages)

# plot the data
fig, ax = plt.subplots()
for j, algorithm in enumerate(algorithms):
  # plot the entire line
  line = (averages[j][:])
  ax.plot(thread_counts, line, linewidth=2, marker='>')

ax.set_ylabel("IDs/ms")
ax.set_xlabel("Thread Count")
ax.set_title("ID Throughput")
ax.legend(algorithms)
plt.show()