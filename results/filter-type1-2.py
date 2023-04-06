import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys


def read_filter(file):
	stream = open(file, 'r')
	filterSet = {}
	while True:
		a = stream.readLine().trim()
		if not a:
			break
		else:
			filterSet.add(int(a))
	stream.close()
	return filterSet


def split_file(file, filterSet):

	file_type1 = file.replace("type1-2", "type1")
	file_type2 = file.replace("type1-2", "type2")

	stream = open(file, 'r')
	stream_type1 = open(file_type1, 'w')
	stream_type2 = open(file_type2, 'w')
	i = 1
	while True:
		a = stream.readLine()
		if not a:
			break
		else:
			if i in filterSet:
				stream_type1.write(a)
			else:
				stream_type2.write(a)
			i = i +1
	stream_type1.close()
	stream_type2.close()


def get_files(directory):
	files = []
	for root, dirs, files in os.walk(directory, topdown=True):
		for name in files:
			files.append(os.path.join(root, name))
		for name in dirs:
			files.append(os.path.join(root, name))
	return files

def type12files(files):
	ft12 = []
	for f in files:
		if "type1-2" in f:
			ft12.append(f)
	return ft12


filter = sys.argv[1]
dir = sys.argv[2]

filterSet = read_filter(filter)
files = get_files(dir)
ft12 = type12files(files)
for f in ft12:
	split_file(f, filterSet)