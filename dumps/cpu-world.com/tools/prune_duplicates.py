#!/usr/bin/env python

import os, glob
import hashlib

def md5_for_file(f, block_size=2**20):
    md5 = hashlib.md5()
    while True:
        data = f.read(block_size)
        if not data:
            break
        md5.update(data)
    return md5.hexdigest()

def intpart(filename):
	filename = os.path.basename(filename)
	return int(filename.split('.')[0])

def comparator(a, b):
	a = intpart(a)
	b = intpart(b)
	if a < b:
		return -1
	if a > b:
		return 1
	return 0

def skip_filename(original):
	d, f = os.path.split(original)
	f, e = os.path.splitext(f)
	f = f + '.skip'
	return os.path.join(d, f)

if __name__ == "__main__":
	md5dict = dict()
	for infile in glob.glob(os.path.join(os.getcwd(), '*.txt')):
		f = open(infile, 'rb')
		md5sum = md5_for_file(f)
		if not md5sum in md5dict:
			md5dict[md5sum] = []
		md5dict[md5sum].append(infile)
		
	for key in md5dict:
		files = md5dict[key]
		if len(files) > 1:
			files.sort(comparator)
			for file in files[1:]:
				f = open(skip_filename(file), "wb")
				f.close()
				os.remove(file)
