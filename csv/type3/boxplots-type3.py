import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



plt.rcParams.update({'font.size': 16})

limit = 600

def to_seconds(value):
	return value / 1000000000.0

#xaxis = float(sys.argv[3])
df_ring = pd.read_csv("type3.ring.fixed.1000.time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_muthu = pd.read_csv("type3.ring-muthu.fixed.1000.time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_random = pd.read_csv("type3.ring.random.1000.time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_random_lonely = pd.read_csv("type3.ring.random-lonely.1000.time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_random_lonely_est = pd.read_csv("type3.ring.random-lonely-est.1000.time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_best = pd.read_csv("type3.ring.best.1000.time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time'])
#df_compactltj = pd.read_csv("type3.cltj.1000.time.csv" ,
#					  header=None, delimiter=';', names=['id', 'res', 'time'])



df_data = pd.DataFrame()
df_data['VEO-R'] = df_ring_random['time'].div( 1000000.0)
df_data['VEO-RL'] = df_ring_random_lonely['time'].div( 1000000.0)
df_data['VEO-RLW'] = df_ring_random_lonely_est['time'].div( 1000000.0)
df_data['VRing-large'] = df_ring_muthu['time'].div( 1000000.0)
df_data['Ring-large'] = df_ring['time'].div( 1000000.0)
df_data['VEO-Best'] = df_ring_best['time'].div( 1000000.0)
#df_data['CompactLTJ'] = df_ring['time'].div( 1000000.0)



names = [ 'RingR', 'RingRNL', 'RingRE', 'VRing', 'Ring', 'RingB']



fig, ax1 = plt.subplots(nrows=1, ncols=1, figsize=(10, 5))
#fig, ax1 = plt.subplots(nrows=1, ncols=1, figsize=(8, 6))

# rectangular box plot
bplot1 = ax1.boxplot(df_data,
					 widths=0.25,
                     patch_artist=True,  # fill with color
                     showfliers=False)  # will be used to label x-ticks
#ax1.set_title('VEOs', weight='bold')

colors = ['cornflowerblue', 'orange', 'mediumseagreen', 'plum', 'silver', 'salmon']

i = 0
for bplot in bplot1['boxes']:
	bplot.set_facecolor(colors[i] )
	bplot.set_edgecolor('black')
	i = i + 1
i = 0
for bplot in bplot1['medians']:
	bplot.set_linewidth(1.5)
	bplot.set_color('red')
	i = i + 1
i = 0
for bplot in bplot1['caps']:
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1

#fig = df_data.boxplot(positions=bpt, grid=False, return_type='axes')
#fig.plot()
#plt.suptitle(title)
ax1.set_xticks(np.arange(1, 7, step=1), names)
ax1.set_ylim(top=1000)
ax1.set_ylim(bottom=-8)
#ax1.set_ylim(bottom=-0.2)
#ax1.set_yscale('symlog')
#ax1.set_yscale('log')

ax1.set_ylabel("Time (ms)")


#fig.legend(bplot1['boxes'], names, ncol=5, fontsize=14, loc='upper center', bbox_to_anchor=[0.5, 1.05])
fig.tight_layout()
#plt.show()


fig.savefig('boxplots-type3.pdf', bbox_inches ="tight")

print(df_data.describe())
print(df_data.median())
#print "Errors"

#for e in errors:
#	print "---"+str(int(e)) + "---"
#	print "New:"
#	print df_new.loc[e-1]
#	print "\n"
#	print "Baseline:"
#	print df_baseline.loc[e-1]
#	print "\n"
