import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys

TO_MSEC = 1000000.0
TO_SEC  = 1000000000.0


TRANSFORM = TO_SEC
if sys.argv[1] == "msec":
	TRANSFORM = TO_MSEC

l = ""
b = ""
if len(sys.argv) > 2:
	l = sys.argv[2]
	b = ".1000"

def to_msec(value):
	return value / 1000000.0

ring_small = []
ring_large = []
rdfcsa_large = []
milldb = []

for t in range(1, 4):
	df_ring = pd.read_csv("adaptive/" + l + "type"+str(t)+".ring"+b+".time.csv" ,
						  header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
	df_cring = pd.read_csv("adaptive/" + l + "type"+str(t)+".c-ring"+b+".time.csv",
						   header=None, delimiter=';', names=['id', 'res', 'time', 'utime'])
	df_rdfcsa = pd.read_csv("adaptive/" + l + "type"+str(t)+".rdfcsa"+b+".time.csv",
							header=None, delimiter=';', names=['id', 'res', 'time'])
	df_milldb = pd.read_csv("milldb/adaptive/" + l + "type"+str(t)+".milldb.csv",
							header=None, delimiter=';', names=['id', 'res', 'time'])

	df_data = pd.DataFrame()
	df_data['Ring-large'] = df_ring['time'].divide( TRANSFORM)
	df_data['Ring-small'] = df_cring['time'].divide( TRANSFORM)
	df_data['RDFCSA-large'] = df_rdfcsa['time'].divide( TRANSFORM)
	df_data['MillDB'] = df_milldb['time'].divide( TRANSFORM)

	ring_small.append(df_data['Ring-small'].mean())
	ring_small.append(df_data['Ring-small'].median())

	ring_large.append(df_data['Ring-large'].mean())
	ring_large.append(df_data['Ring-large'].median())

	rdfcsa_large.append(df_data['RDFCSA-large'].mean())
	rdfcsa_large.append(df_data['RDFCSA-large'].median())

	milldb.append(df_data['MillDB'].mean())
	milldb.append(df_data['MillDB'].median())


str_ring_small = ""
for v in ring_small:
	str_ring_small = str_ring_small + " & " + str(v)

str_ring_large = ""
for v in ring_large:
	str_ring_large = str_ring_large + " & " + str(v)

str_rdfcsa_large = ""
for v in rdfcsa_large:
	str_rdfcsa_large =  str_rdfcsa_large + " & " + str(v)

str_milldb = ""
for v in milldb:
	str_milldb =  str_milldb + " & " + str(v)


print(str_ring_small)
print(str_ring_large)
print(str_rdfcsa_large)
print(str_milldb)






