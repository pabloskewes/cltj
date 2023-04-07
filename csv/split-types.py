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
			listData.append(a)
	stream.close()
	return listData

def replace_index(index, data):
	i = data.find(';')
	return str(index) + ";" + data[i+1:]

def classify(file, file1, file2, file3, list_types):
	stream = open(file, 'r')
	stream_type1 = open(file1, 'w')
	stream_type2 = open(file2, 'w')
	stream_type3 = open(file3, 'w')
	i = 0
	while True:
		a = stream.readline().strip()
		if not a:
			break
		else:
			if list_types[i] == 'TYPE1':
				stream_type1.write(a + "\n")
			elif list_types[i] == 'TYPE2':
				stream_type2.write(a + "\n")
			else:
				stream_type3.write(a + "\n")
		i = i + 1
	stream.close()
	stream_type1.close()
	stream_type2.close()
	stream_type3.close()


def get_files(directory):
	adaptive = glob.glob(directory + "adaptive/**/*.csv", recursive=True)
	fixed = glob.glob(directory + "fixed/**/*.csv", recursive=True)
	return fixed + adaptive


def name_file(file):
	i = file.rfind('/')
	a = file[i+1:]
	return a

def dir_file(file):
	i = file.rfind('/')
	a = file[:i+1]
	return a

def type1_file(f, d):
	return d + 'type1.' + f 

def type2_file(f, d):
	return d + 'type2.' + f 

def type3_file(f, d):
	return d + 'type3.' + f 


d = sys.argv[1]

list_types = read_list('types.txt')
files = get_files(d)
for f in files:
	name = name_file(f)
	d = dir_file(f)
	t1 = type1_file(name, d)
	t2 = type2_file(name, d)
	t3 = type3_file(name, d)
	classify(f, t1, t2, t3, list_types)
