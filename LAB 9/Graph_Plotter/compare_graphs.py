# General Python script for generating comparison graphs (-Mitesh K)

#!/usr/bin/python

import matplotlib.pyplot as plt
import csv, sys, re, random, os, time
import numpy as np
########## Plotting section
plt.figure(figsize=(10, 8))

data = """
# data to be plotted in the <xi yi> format goes here
# can also take from text file
"""

# Split the data into lines
lines = data.split('\n')

# Split lines into two graphs (assuming 19 data points for each graph)
graph1_data = lines[:]

# Extract x and y values for each graph
x1_values, y1_values = zip(*[map(float, line.split()) for line in graph1_data])

# Convert the extracted values to lists
x1_values = list(x1_values)
y1_values = list(y1_values)

plt.plot(x1_values, y1_values, label="label", ls="--", color="red", marker="x", markersize=9, linewidth=2, markeredgewidth=2)

plt.xlabel('xLabel')
plt.ylabel('yLabel')

plt.grid('on')
plt.legend(loc='best')
plt.show()
plt.savefig("fig_name.png")