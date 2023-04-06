import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys


limit = 600

def to_seconds(value):
	return value / 1000000000.0
a = ""
fix_adap = sys.argv[1]
if fix_adap == "fixed":
	a = ".fixed"
l = ""
b = ""
if len(sys.argv) > 2:
	l = sys.argv[2]
	b = ".1000"

#xaxis = float(sys.argv[3])
df_ring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type1-2.ring"+a+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type1-2.c-ring"+a+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type1-2.uring"+a+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type1-2.c-uring"+a+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa = pd.read_csv("rdfcsa/csv/" + fix_adap + "/" + l + "type1-2.rdfcsa"+a+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa = pd.read_csv("rdfcsa/csv/" + fix_adap + "/" + l + "type1-2.crdfcsa"+a+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])


df_data = pd.DataFrame()
df_data['Ring-large'] = df_ring['time'].div( 1000000.0)
df_data['Ring-small'] = df_cring['time'].div( 1000000.0)
df_data['URing-large'] = df_uring['time'].div( 1000000.0)
df_data['URing-small'] = df_curing['time'].div( 1000000.0)
df_data['RDFCSA-large'] = df_rdfcsa['time'].div( 1000000.0)
df_data['RDFCSA-small'] = df_crdfcsa['time'].div( 1000000.0)

names = ['Ring-large', 'Ring-small', 'URing-large', 'URing-small', 'RDFCSA-large', 'RDFCSA-small']
bpt = [12.15, 7.295, 23.529, 14.613, 23.543, 15.846]

title = "Type 1-2 "
if fix_adap == "fixed":
	title = title + " Fixed "
else:
	title = title + " Adaptive "
if l == "limit/":
	title = title + " Limit"

print("--------------------------------------------------")
print(title)
print("Name;Mean;Avg;Timeout")
for name in names:
	col = df_data[name]
	print(name + ";" + str(col.median()) + ";" + str(col.mean()) + ";" +
		  str(col[col >= limit].count()))

print("--------------------------------------------------")


fig = df_data.boxplot(positions=bpt, patch_artist=True, grid=False, return_type='axes')
fig.plot()
plt.suptitle(title)
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
