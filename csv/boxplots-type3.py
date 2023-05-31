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



df_data = pd.DataFrame()
df_data['Ring-large'] = df_ring['time'].div( 1000000.0)
df_data['VRing-large'] = df_ring_muthu['time'].div( 1000000.0)
df_data['VEO-R'] = df_ring_random['time'].div( 1000000.0)
df_data['VEO-RL'] = df_ring_random_lonely['time'].div( 1000000.0)
df_data['VEO-RLW'] = df_ring_random_lonely_est['time'].div( 1000000.0)
df_data['VEO-Best'] = df_ring_best['time'].div( 1000000.0)

names = ['Ring-large', 'VRing-large', 'VEO-R', 'VEO-RL', 'VEO-RLW', 'VEO-Best']



fig, ax1 = plt.subplots(nrows=1, ncols=1, figsize=(10, 16))

# rectangular box plot
bplot1 = ax1.boxplot(df_data,
                     patch_artist=True,  # fill with color
                     showfliers=False)  # will be used to label x-ticks
ax1.set_title('VEOS', weight='bold')

colors = ['cornflowerblue', 'orange', 'mediumseagreen', 'plum', 'silver', 'red']

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
ax1.set_xticks(np.arange(6, 25, step=2), np.arange(6, 25, step=2))
ax1.set_ylim(top=70)
ax1.set_ylim(bottom=-1)
#ax1.set_ylim(bottom=-0.2)
#ax1.set_yscale('symlog')
#ax1.set_yscale('log')

ax1.set_ylabel("Time (ms)")
ax1.set_xlabel("Space (bpt)")


#df_ring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.ring"+a+b+".time.csv" ,
#					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_cring = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-ring"+a+b+".time.csv",
#					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_uring = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.uring"+a+b+".time.csv" ,
#					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_curing = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-uring"+a+b+".time.csv",
#					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_ringm = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.ring-muthu"+a+b+".time.csv" ,
#					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_cringm = pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-ring-muthu"+a+b+".time.csv",
#					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_uringm = pd.read_csv("ring/csv/" + fix_adap + "/" + l + "type3.uring-muthu"+a+b+".time.csv" ,
#					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_curingm= pd.read_csv("cring/csv/" + fix_adap + "/" + l + "type3.c-uring-muthu"+a+b+".time.csv",
#						header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
#df_rdfcsa = pd.read_csv("rdfcsa/csv/" + fix_adap + "/" + l + "type3.rdfcsa"+a+b+".time.csv",
#						header=None, delimiter=';', names=['id', 'res', 'time'])
#df_crdfcsa = pd.read_csv("rdfcsa/csv/" + fix_adap + "/" + l + "type3.crdfcsa"+a+b+".time.csv",
#						header=None, delimiter=';', names=['id', 'res', 'time'])#
#

#names = ['Ring-large', 'Ring-small', 'URing-large', 'URing-small', 'VRing-large',  'VRing-small', 'VURing-large', 'VURing-small', 'RDFCSA-large', 'RDFCSA-small']
#bpt = [12.15, 7.30, 23.0, 14.61, 40.28, 35.42, 51.65, 42.74, 24.0, 15.81]#

#df_data = pd.DataFrame()
#df_data['Ring-large'] = df_ring['time'].div( 1000000.0)
#df_data['Ring-small'] = df_cring['time'].div( 1000000.0)
#df_data['URing-large'] = df_uring['time'].div( 1000000.0)
#df_data['URing-small'] = df_curing['time'].div( 1000000.0)
#df_data['VRing-large'] = df_ringm['time'].div( 1000000.0)
#df_data['VRing-small'] = df_cringm['time'].div( 1000000.0)
#df_data['VURing-large'] = df_uringm['time'].div( 1000000.0)
#df_data['VURing-small'] = df_curingm['time'].div( 1000000.0)
#df_data['RDFCSA-large'] = df_rdfcsa['time'].div( 1000000.0)
#df_data['RDFCSA-small'] = df_crdfcsa['time'].div( 1000000.0)#

## rectangular box plot
#bplot3 = ax3.boxplot(df_data,
#					 widths = 1,
#                     patch_artist=True,  # fill with color
#                     positions = bpt)  # will be used to label x-ticks
#ax3.set_title('Type III')#

#i = 0
#for bplot in bplot3['medians']:
#	#print(bplot)
#	#bplot.set_color(colors2[i])
#	bplot.set_color('black')
#	bplot.set_linewidth(1.5)
#	i = i + 1
#i = 0
#for bplot in bplot3['whiskers']:
#	print(bplot)
#	#bplot.set_color(colors[i//2])
#	bplot.set_color('grey')
#	i = i + 1
#i = 0
#for bplot in bplot3['caps']:
#	print(bplot)
#	#bplot.set_color(colors[i//2])
#	bplot.set_color('grey')
#	i = i + 1
#i = 0
#for bplot in bplot3['fliers']:
#	print(bplot)
#	bplot.set_markeredgecolor(colors2[i])
#	i = i + 1
#i = 0
#for bplot in bplot3['boxes']:
#	print(bplot)
#	bplot.set_facecolor(colors2[i])
#	#bplot.set_edgecolor(colors[i])
#	bplot.set_edgecolor('grey')
#	i = i + 1#


#ax3.set_xticks(np.arange(6, 56, step=4), np.arange(6, 56, step=4))
##ax2.set_ylim(top=2000)
##ax3.set_ylim(bottom=-0.2)
##ax3.set_yscale('symlog')
#ax3.set_yscale('log')#

#ax3.set_ylabel("Time (ms)")
#ax3.set_xlabel("Space (bpt)")

#fig.legend(bplot1['boxes'], names, loc='outside upper center', ncol=5)
#fig.legend(bplot3['boxes'][::2], names[::2], ncol=5, fontsize=12, loc='upper center')
fig.legend(bplot1['boxes'], names, ncol=5, fontsize=14, loc='upper center', bbox_to_anchor=[0.5, 1.05])
fig.tight_layout()
#plt.show()


fig.savefig('boxplots-type3.pdf', bbox_inches ="tight")

#print "Errors"

#for e in errors:
#	print "---"+str(int(e)) + "---"
#	print "New:"
#	print df_new.loc[e-1]
#	print "\n"
#	print "Baseline:"
#	print df_baseline.loc[e-1]
#	print "\n"
