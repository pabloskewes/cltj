import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys
import glob


def read_list(file):
	stream = open(file, 'r')
	listData = []
	while True:
		a = stream.readline().strip()
		if not a:
			break
		else:
			listData.append(int(a))
	stream.close()
	return listData

def replace_index(index, data):
	i = data.find(';')
	return str(index) + ";" + data[i+1:]

def join_file(file12, file3, list12, list3, finalfile):
	values = []
	stream_type12 = open(file12, 'r')
	i = 0
	while True:
		a = stream_type12.readline().strip()
		if not a:
			break
		else:
			values.append((list12[i], a))
		i = i + 1
	stream_type12.close()

	stream_type3 = open(file3, 'r')
	i = 0
	while True:
		a = stream_type3.readline().strip()
		if not a:
			break
		else:
			values.append((list3[i], a))
		i = i + 1
	stream_type3.close()
	values.sort(key=lambda a: a[0])
	stream_final = open(finalfile, 'w')
	for v in values:
		stream_final.write(replace_index(v[0], v[1]) + "\n")
	stream_final.close()


def get_type12files(directory):
	return glob.glob(directory + "/**/type1-2*.csv", recursive=True)

def get_type3files(directory):
	return glob.glob(directory + "/**/type3*.csv", recursive=True)

def get_type3(nameType12):
	return nameType12.replace('type1-2', 'type3')

def name_file(nameType12):
	i = nameType12.rfind('/')
	a = nameType12[i+1:]
	a = a.replace('type1-2.', '')
	return a

def folder_file(nameType12):
	folder = ''
	if 'adaptive' in nameType12:
		folder = folder + "adaptive/"
	else:
		folder = folder + "fixed/"
	if 'limit' in nameType12:
		folder = folder + "limit/"
	return folder


d = sys.argv[1]

list12 = read_list('map-type12.txt')
list3 = read_list('map-type3.txt')
files_t12 = get_type12files(d)
for file12 in files_t12:
	finalfile = folder_file(file12) + name_file(file12)
	file3 = get_type3(file12)
	print(file12 + " " + file3)
	join_file(file12, file3, list12, list3, finalfile)

