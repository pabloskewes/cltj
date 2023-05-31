import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys


limit = 600

def to_seconds(value):
	return value / 1000000000.0

name1 = sys.argv[1]
name2 = sys.argv[2]
#xaxis = float(sys.argv[3])
df_rdfcsa = pd.read_csv(sys.argv[1], header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring = pd.read_csv(sys.argv[2], header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
ratio = "random/size"

df_data = pd.DataFrame()
df_data['random'] = df_rdfcsa['time'].div( 1000000.0)
df_data['size'] = df_ring['time'].div( 1000000.0)
df_data[ratio] = df_data['random'] / df_data['size']


col_ring = df_data['random']
print("***** random *****")
sec_median = col_ring.median()
print("Median: " + str(sec_median))
print("Avg: " + str(col_ring.mean()))
timeout = col_ring[col_ring >= limit].count()
print("Timeout: " + str(timeout))

col_rdfcsa = df_data['size']
print("***** size *****")
sec_median = col_rdfcsa.median()
print("Median: " + str(sec_median))
print("Avg: " + str(col_rdfcsa.mean()))
timeout = col_rdfcsa[col_rdfcsa >= limit].count()
print("Timeout: " + str(timeout))

col_ratio = df_data[ratio]
print("*****"+ratio+"*****")
print("Median: " + str(col_ratio.median()))
print("Avg: " + str(col_ratio.mean()))


fig, (ax1, ax2) = plt.subplots(1, 2)
fig.suptitle( "random vs size")

axes = df_data.boxplot(column=['random', 'size'], grid=False, return_type='axes', ax=ax1, whis=[5, 95])
#b = axes.get_ylim()[0]
#axes.set(ylim=(b, xaxis))
df_data.boxplot(column=[ratio], grid=False, return_type='axes', ax=ax2, whis=[5, 95])
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
