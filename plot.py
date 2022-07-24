import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df_data = pd.read_csv('refactoring.csv', sep=';')
print(df_data)
print(df_data.head())
axes = df_data.boxplot(column=['Orig', 'V1', 'V2', 'V3'], grid=False, return_type='axes')
#axes = df_data.boxplot(column=['Intersection'], grid=False, return_type='axes')

#plt.ylim(0, 40000000)
plt.ylabel("Time (ns)")
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
