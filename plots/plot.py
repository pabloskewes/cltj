import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df_data = pd.read_csv('bgps.comp.csv', sep=';')
print(df_data.describe())
#print(df_data.head())
axes = df_data.boxplot(column=['Comp'], grid=False, return_type='axes')
#axes = df_data.boxplot(column=['Intersection'], grid=False, return_type='axes')

plt.ylim(0, 30)
#plt.ylabel("Time (ns)")
plt.ylabel("Ratio")
plt.xlabel("")
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
