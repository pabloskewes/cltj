import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



limit = 600000000000.0
file = sys.argv[1]
msec_sec = sys.argv[2]

def to_msec(value):
	return value / 1000000.0

def to_sec(value):
	return value / 1000000000.0


#xaxis = float(sys.argv[3])
df = pd.read_csv(file ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
col_time = df['time']
print("*****"+file+"*****")
sec_median = col_time.median()
if msec_sec == 's':
	print("Median: " + str(to_sec(sec_median)))
	print("Avg: " + str(to_sec(col_time.mean())))
	print("Min: " + str(to_sec(col_time.min())))
	print("Max: " + str(to_sec(col_time.max())))

	print('95: ' + str(to_sec(col_time.quantile(0.95))))
	print('75: ' + str(to_sec(col_time.quantile(0.75))))
	print('50: ' + str(to_sec(col_time.quantile(0.5))))
	print('25: ' + str(to_sec(col_time.quantile(0.25))))
	print(' 5: ' + str(to_sec(col_time.quantile(0.05))))
else:
	print("Median: " + str(to_msec(sec_median)))
	print("Avg: " + str(to_msec(col_time.mean())))
	print("Min: " + str(to_msec(col_time.min())))
	print("Max: " + str(to_msec(col_time.max())))

	print('95: ' + str(to_msec(col_time.quantile(0.95))))
	print('75: ' + str(to_msec(col_time.quantile(0.75))))
	print('50: ' + str(to_msec(col_time.quantile(0.5))))
	print('25: ' + str(to_msec(col_time.quantile(0.25))))
	print(' 5: ' + str(to_msec(col_time.quantile(0.05))))

timeout = col_time[col_time >= limit].count()
print("Timeout: " + str(timeout))

