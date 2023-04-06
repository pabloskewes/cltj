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
df_ring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.ring"+a+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-ring"+a+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.uring"+a+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-uring"+a+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ringm = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.ring-muthu"+a+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cringm = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-ring-muthu"+a+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uringm = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.uring-muthu"+a+b+".time.csv" ,
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curingm= pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-uring-muthu"+a+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])



df_data = pd.DataFrame()
df_data['Ring'] = df_ring['time'].div( 1000000000.0)
df_data['CRing'] = df_cring['time'].div( 1000000000.0)
df_data['URing'] = df_uring['time'].div( 1000000000.0)
df_data['CURing'] = df_curing['time'].div( 1000000000.0)
df_data['VRing'] = df_ringm['time'].div( 1000000000.0)
df_data['VCRing'] = df_cringm['time'].div( 1000000000.0)
df_data['VURing'] = df_uringm['time'].div( 1000000000.0)
df_data['VCURing'] = df_curingm['time'].div( 1000000000.0)

names = ['Ring', 'CRing', 'URing', 'CURing', 'VRing', 'VCRing', 'VURing', 'VCURing']

title = "Type 3 "
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


fig = df_data.boxplot(column=names, grid=False, return_type='axes')
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
