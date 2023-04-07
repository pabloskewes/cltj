import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



plt.rcParams.update({'font.size': 16})

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
df_ring = pd.read_csv("adaptive/" + l + "type1.ring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring = pd.read_csv("adaptive/" + l + "type1.c-ring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring = pd.read_csv("adaptive/" + l + "type1.uring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing = pd.read_csv("adaptive/" + l + "type1.c-uring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa = pd.read_csv("adaptive/" + l + "type1.rdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa = pd.read_csv("adaptive/" + l + "type1.crdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_ring_fix = pd.read_csv("fixed/" + l + "type1.ring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring_fix = pd.read_csv("fixed/" + l + "type1.c-ring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring_fix = pd.read_csv("fixed/" + l + "type1.uring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing_fix = pd.read_csv("fixed/" + l + "type1.c-uring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa_fix = pd.read_csv("fixed/" + l + "type1.rdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa_fix = pd.read_csv("fixed/" + l + "type1.crdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])


df_data = pd.DataFrame()
df_data['Ring-large'] = df_ring['time'].div( 1000000000.0)
df_data['Ring-large-fix'] = df_ring_fix['time'].div( 1000000000.0)
df_data['Ring-small'] = df_cring['time'].div( 1000000000.0)
df_data['Ring-small-fix'] = df_cring_fix['time'].div( 1000000000.0)
df_data['URing-large'] = df_uring['time'].div( 1000000000.0)
df_data['URing-large-fix'] = df_uring_fix['time'].div( 1000000000.0)
df_data['URing-small'] = df_curing['time'].div( 1000000000.0)
df_data['URing-small-fix'] = df_curing_fix['time'].div( 1000000000.0)
df_data['RDFCSA-large'] = df_rdfcsa['time'].div( 1000000000.0)
df_data['RDFCSA-fix'] = df_rdfcsa_fix['time'].div( 1000000000.0)
df_data['RDFCSA-small'] = df_crdfcsa['time'].div( 1000000000.0)
df_data['RDFCSA-small-fix'] = df_crdfcsa_fix['time'].div( 1000000000.0)

names = ['Ring-large', 'Ring-small', 'URing-large', 'URing-small', 'RDFCSA-large', 'RDFCSA-small']
bpt = [12, 12.5, 7.30, 7.8, 22.2, 22.7, 14.61, 15.11, 23.2, 23.7, 15.8, 16.3]

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


fig, (ax1, ax2, ax3) = plt.subplots(nrows=3, ncols=1, figsize=(10, 16))

# rectangular box plot
bplot1 = ax1.boxplot(df_data,
                     patch_artist=True,  # fill with color
                     positions = bpt,
                     showfliers=False)  # will be used to label x-ticks
ax1.set_title('Type I', weight='bold')

colors = ['cornflowerblue', 'orange', 'mediumseagreen', 'plum', 'silver', 'chocolate']
colors2 = ['cornflowerblue', 'orange', 'mediumseagreen', 'plum', 'lightblue', 'bisque', 'springgreen', 'pink', 'silver', 'chocolate']

print(bplot1)
i = 0
for bplot in bplot1['boxes']:
	print(bplot)
	bplot.set_facecolor(colors[i // 2] )
	if(i % 2)==1:
		bplot.set_hatch('//')
	#bplot.set_edgecolor(colors[i])
	bplot.set_edgecolor('black')
	i = i + 1
i = 0
for bplot in bplot1['medians']:
	print(bplot)
	#bplot.set_color(colors[i])
	bplot.set_linewidth(1.5)
	bplot.set_color('red')
	i = i + 1
i = 0
for bplot in bplot1['whiskers']:
	print(bplot)
	if(i//2)%2 == 1:
		bplot.set_linestyle('--')
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot1['caps']:
	print(bplot)
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot1['fliers']:
	print(bplot)
	if i%2 == 0:
		bplot.set_markerfacecolor(colors[i // 2])
		bplot.set_markeredgecolor('black')
	else:
		bplot.set_markeredgecolor(colors[i // 2])
	i = i + 1

#fig = df_data.boxplot(positions=bpt, grid=False, return_type='axes')
#fig.plot()
#plt.suptitle(title)
ax1.set_xticks(np.arange(6, 25, step=2), np.arange(6, 25, step=2))
ax1.set_ylim(top=9)
ax1.set_ylim(bottom=-0.2)
#ax1.set_ylim(bottom=-0.2)
#ax1.set_yscale('symlog')
#ax1.set_yscale('log')

ax1.set_ylabel("Time (s)")
ax1.set_xlabel("Space (bpt)")

df_ring = pd.read_csv("adaptive/" + l + "type2.ring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring = pd.read_csv("adaptive/" + l + "type2.c-ring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring = pd.read_csv("adaptive/" + l + "type2.uring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing = pd.read_csv("adaptive/" + l + "type2.c-uring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa = pd.read_csv("adaptive/" + l + "type2.rdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa = pd.read_csv("adaptive/" + l + "type2.crdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_ring_fix = pd.read_csv("fixed/" + l + "type2.ring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring_fix = pd.read_csv("fixed/" + l + "type2.c-ring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring_fix = pd.read_csv("fixed/" + l + "type2.uring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing_fix = pd.read_csv("fixed/" + l + "type2.c-uring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa_fix = pd.read_csv("fixed/" + l + "type2.rdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa_fix = pd.read_csv("fixed/" + l + "type2.crdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])


df_data = pd.DataFrame()
df_data['Ring-large'] = df_ring['time'].div( 1000000000.0)
df_data['Ring-large-fix'] = df_ring_fix['time'].div( 1000000000.0)
df_data['Ring-small'] = df_cring['time'].div( 1000000000.0)
df_data['Ring-small-fix'] = df_cring_fix['time'].div( 1000000000.0)
df_data['URing-large'] = df_uring['time'].div( 1000000000.0)
df_data['URing-large-fix'] = df_uring_fix['time'].div( 1000000000.0)
df_data['URing-small'] = df_curing['time'].div( 1000000000.0)
df_data['URing-small-fix'] = df_curing_fix['time'].div( 1000000000.0)
df_data['RDFCSA-large'] = df_rdfcsa['time'].div( 1000000000.0)
df_data['RDFCSA-fix'] = df_rdfcsa_fix['time'].div( 1000000000.0)
df_data['RDFCSA-small'] = df_crdfcsa['time'].div( 1000000000.0)
df_data['RDFCSA-small-fix'] = df_crdfcsa_fix['time'].div( 1000000000.0)

# rectangular box plot
bplot2 = ax2.boxplot(df_data,
                     patch_artist=True,  # fill with color
                     positions = bpt,
                     showfliers=False)  # will be used to label x-ticks
ax2.set_title('Type II', weight='bold')

i = 0
for bplot in bplot2['boxes']:
	print(bplot)
	bplot.set_facecolor(colors[i // 2] )
	if(i % 2)==1:
		bplot.set_hatch('//')
	#bplot.set_edgecolor(colors[i])
	bplot.set_edgecolor('black')
	i = i + 1
i = 0
for bplot in bplot2['medians']:
	print(bplot)
	#bplot.set_color(colors[i])
	bplot.set_linewidth(1.5)
	bplot.set_color('red')
	i = i + 1
i = 0
for bplot in bplot2['whiskers']:
	print(bplot)
	if(i//2)%2 == 1:
		bplot.set_linestyle('--')
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot2['caps']:
	print(bplot)
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot2['fliers']:
	print(bplot)
	if i%2 == 0:
		bplot.set_markerfacecolor(colors[i // 2])
		bplot.set_markeredgecolor('black')
	else:
		bplot.set_markeredgecolor(colors[i // 2])
	i = i + 1

i = 0
for bplot in bplot2['caps']:
	captop = int(bplot.get_ydata()[0])
	if captop > 150:
		if (i//2) % 2 == 0:
			#xlabel = bpt[i // 2]-5.1
			#ax3.text(xlabel, 1053,
	        #    '{:d}'.format(captop), va='center', weight='bold')
			xlabel = bpt[i // 2]-0.5
			ax2.text(xlabel, 130,
	            '{:d}'.format(captop), va='center', rotation=90)
		else:
			xlabel = bpt[i // 2]+0.1
			ax2.text(xlabel, 130,
	            '{:d}'.format(captop), va='center', rotation=90)
	i = i + 1

#fig = df_data.boxplot(positions=bpt, grid=False, return_type='axes')
#fig.plot()
#plt.suptitle(title)
ax2.set_xticks(np.arange(6, 25, step=2), np.arange(6, 25, step=2))
#ax1.set_ylim(top=150)
ax2.set_ylim(top=150)
ax2.set_ylim(bottom=-2)
#ax2.set_ylim(bottom=-0.2)
#ax2.set_yscale('symlog')

ax2.set_ylabel("Time (s)")
ax2.set_xlabel("Space (bpt)")

df_ring = pd.read_csv("adaptive/" + l + "type3.ring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring = pd.read_csv("adaptive/" + l + "type3.c-ring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring = pd.read_csv("adaptive/" + l + "type3.uring"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing = pd.read_csv("adaptive/" + l + "type3.c-uring"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa = pd.read_csv("adaptive/" + l + "type3.rdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa = pd.read_csv("adaptive/" + l + "type3.crdfcsa"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_ring_fix = pd.read_csv("fixed/" + l + "type3.ring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring_fix = pd.read_csv("fixed/" + l + "type3.c-ring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring_fix = pd.read_csv("fixed/" + l + "type3.uring.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing_fix = pd.read_csv("fixed/" + l + "type3.c-uring.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_rdfcsa_fix = pd.read_csv("fixed/" + l + "type3.rdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])
df_crdfcsa_fix = pd.read_csv("fixed/" + l + "type3.crdfcsa.fixed"+b+".time.csv",
						header=None, delimiter=';', names=['id', 'res', 'time'])

df_ringm = pd.read_csv("adaptive/" + l + "type3.ring-muthu"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cringm = pd.read_csv("adaptive/" + l + "type3.c-ring-muthu"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uringm = pd.read_csv("adaptive/" + l + "type3.uring-muthu"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curingm = pd.read_csv("adaptive/" + l + "type3.c-uring-muthu"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_ring_fixm = pd.read_csv("fixed/" + l + "type3.ring-muthu.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_cring_fixm = pd.read_csv("fixed/" + l + "type3.c-ring-muthu.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_uring_fixm = pd.read_csv("fixed/" + l + "type3.uring-muthu.fixed"+b+".time.csv" ,
					  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
df_curing_fixm = pd.read_csv("fixed/" + l + "type3.c-uring-muthu.fixed"+b+".time.csv",
					   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])


df_data = pd.DataFrame()
df_data['Ring-large'] = df_ring['time'].divide( 1000000000.0)
df_data['Ring-large-fix'] = df_ring_fix['time'].divide( 1000000000.0)
df_data['Ring-small'] = df_cring['time'].divide( 1000000000.0)
df_data['Ring-small-fix'] = df_cring_fix['time'].divide( 1000000000.0)
df_data['URing-large'] = df_uring['time'].divide( 1000000000.0)
df_data['URing-large-fix'] = df_uring_fix['time'].divide( 1000000000.0)
df_data['URing-small'] = df_curing['time'].divide( 1000000000.0)
df_data['URing-small-fix'] = df_curing_fix['time'].divide( 1000000000.0)
df_data['VRing-large'] = df_ringm['time'].divide( 1000000000.0)
df_data['VRing-large-fix'] = df_ring_fixm['time'].divide( 1000000000.0)
df_data['VRing-small'] = df_cringm['time'].divide( 1000000000.0)
df_data['VRing-small-fix'] = df_cring_fixm['time'].divide( 1000000000.0)
df_data['VURing-large'] = df_uringm['time'].divide( 1000000000.0)
df_data['VURing-large-fix'] = df_uring_fixm['time'].divide( 1000000000.0)
df_data['VURing-small'] = df_curingm['time'].divide( 1000000000.0)
df_data['VURing-small-fix'] = df_curing_fixm['time'].divide( 1000000000.0)
df_data['RDFCSA-large'] = df_rdfcsa['time'].divide( 1000000000.0)
df_data['RDFCSA-large-fix'] = df_rdfcsa_fix['time'].divide( 1000000000.0)
df_data['RDFCSA-small'] = df_crdfcsa['time'].divide( 1000000000.0)
df_data['RDFCSA-small-fix'] = df_crdfcsa_fix['time'].divide( 1000000000.0)

names = ['Ring-large', 'Ring-large-F', 'Ring-small', 'Ring-small-F', 'URing-large', 'URing-large-F', 'URing-small', 'URing-small-F', 
		 'VRing-large', 'VRing-large-F', 'VRing-small', 'VRing-small-F', 'VURing-large', 'VURing-large-F', 'VURing-small', 'VURing-small-F', 
		 'RDFCSA-large', 'RDFCSA-large-F', 'RDFCSA-small', 'RDFCSA-small-F']
#bpt = [12.15, 7.30, 23.0, 14.61, 40.28, 35.42, 51.65, 42.74, 24.0, 15.81]

bpt = [12, 13, 7.30, 8.3, 22.0, 23, 14.6, 15.6, 40.3, 41.3, 35.4, 36.4, 51.7, 52.7, 42.7, 43.7, 24.0, 25.0, 16.6, 17.6]

# rectangular box plot
bplot3 = ax3.boxplot(df_data,
					 widths = 1,
                     patch_artist=True,  # fill with color
                     positions = bpt,
                     showfliers=False)  # will be used to label x-ticks
ax3.set_title('Type III', weight='bold')

i = 0
for bplot in bplot3['boxes']:
	print(bplot)
	bplot.set_facecolor(colors2[i // 2] )
	if(i % 2)==1:
		bplot.set_hatch('//')
	#bplot.set_edgecolor(colors[i])
	bplot.set_edgecolor('black')
	i = i + 1
i = 0
for bplot in bplot3['medians']:
	print(bplot)
	#bplot.set_color(colors[i])
	bplot.set_linewidth(1.5)
	bplot.set_color('red')
	i = i + 1
i = 0
for bplot in bplot3['whiskers']:
	print(bplot)
	if(i//2)%2 == 1:
		bplot.set_linestyle('--')
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot3['caps']:
	print(bplot)
	#bplot.set_color(colors[i//2])
	bplot.set_color('black')
	i = i + 1
i = 0
for bplot in bplot3['fliers']:
	print(bplot)
	if i%2 == 0:
		bplot.set_markerfacecolor(colors2[i // 2])
		bplot.set_markeredgecolor('black')
	else:
		bplot.set_markeredgecolor(colors2[i // 2])
	i = i + 1

#bigotes cortados
i = 0
for bplot in bplot3['caps']:
	captop = int(bplot.get_ydata()[0])
	if captop > 400:
		if (i//2) % 2 == 0:
			#xlabel = bpt[i // 2]-5.1
			#ax3.text(xlabel, 1053,
	        #    '{:d}'.format(captop), va='center', weight='bold')
			xlabel = bpt[i // 2]-1.2
			ax3.text(xlabel, 350,
	            '{:d}'.format(captop), va='center', rotation=90)
		else:
			xlabel = bpt[i // 2]+0.1
			ax3.text(xlabel, 350,
	            '{:d}'.format(captop), va='center', rotation=90)
	i = i + 1



ax3.set_xticks(np.arange(6, 56, step=4), np.arange(6, 56, step=4))
ax3.set_ylim(top=400)
ax3.set_ylim(bottom=-5)
#ax2[0].set_yscale('symlog')
#ax1.set_yscale('log')

ax3.set_ylabel("Time (s)")
ax3.set_xlabel("Space (bpt)")

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
fig.legend(bplot3['boxes'][::2], names[::2], ncol=5, fontsize=14, loc='upper center', bbox_to_anchor=[0.5, 1.05])
fig.tight_layout()
#plt.show()


fig.savefig('boxplots-nolimit.pdf', bbox_inches ="tight")

#print "Errors"

#for e in errors:
#	print "---"+str(int(e)) + "---"
#	print "New:"
#	print df_new.loc[e-1]
#	print "\n"
#	print "Baseline:"
#	print df_baseline.loc[e-1]
#	print "\n"
