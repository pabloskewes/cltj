import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys


limit = 600000000000

def to_seconds(value):
	return value / 1000000000.0

name1 = sys.argv[1]
name2 = sys.argv[2]
#xaxis = float(sys.argv[3])
df_rdfcsa = pd.read_csv(sys.argv[1], header=None, delimiter=';', names=['id', 'res', 'time'])
df_ring = pd.read_csv(sys.argv[2], header=None, delimiter=';', names=['id', 'res', 'time'])
ratio = name1 + "/" + name2

df_data = pd.DataFrame()
df_data[name1] = df_rdfcsa['time']
df_data[name2] = df_ring['time']
df_data[ratio] = df_data[name1] / df_data[name2]


col_ring = df_data[name1]
print("*****"+name1+"*****")
sec_median = to_seconds(col_ring.median())
print("Median: " + str(sec_median))
print("Avg: " + str(to_seconds(col_ring.mean())))
timeout = col_ring[col_ring >= limit].count()
print("Timeout: " + str(timeout))

col_rdfcsa = df_data[name2]
print("*****"+name2+"*****")
sec_median = to_seconds(col_rdfcsa.median())
print("Median: " + str(sec_median))
print("Avg: " + str(to_seconds(col_rdfcsa.mean())))
timeout = col_rdfcsa[col_rdfcsa >= limit].count()
print("Timeout: " + str(timeout))

col_ratio = df_data[ratio]
print("*****"+ratio+"*****")
print("Median: " + str(col_ratio.median()))
print("Avg: " + str(col_ratio.mean()))


fig, (ax1, ax2) = plt.subplots(1, 2)
fig.suptitle(name1 +" vs " + name2)

axes = df_data.boxplot(column=[name1, name2], grid=False, return_type='axes', ax=ax1)
#b = axes.get_ylim()[0]
#axes.set(ylim=(b, xaxis))
df_data.boxplot(column=[ratio], grid=False, return_type='axes', ax=ax2)
#axes = df_data.boxplot(column=['Intersection'], grid=False, return_type='axes')
plt.show()


#print "Errors"

#for e in errors:
#	print "---"+str(int(e)) + "---"
#	print "New:"
#	print df_new.loc[e-1]
#	print "\n"
#	print "Baseline:"
#	print df_baseline.loc[e-1]
#	print "\n"
