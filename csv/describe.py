import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



limit = 600000000000.0
file = sys.argv[1]

def to_msec(value):
	return value / 1000000.0



#xaxis = float(sys.argv[3])
df = pd.read_csv(file ,
					  header=None, delimiter=';', names=['id', 'res', 'time'])
col_time = df['time']
print("*****"+file+"*****")
sec_median = col_time.median()
print("Median: " + str(to_msec(sec_median)))
print("Avg: " + str(to_msec(col_time.mean())))
timeout = col_time[col_time >= limit].count()
print("Timeout: " + str(timeout))