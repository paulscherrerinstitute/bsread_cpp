#!/usr/bin/python
import json
import os
import sys

#Generate json file

names = []

while True:
	line = sys.stdin.readline()
	line = line.strip()
	if not len(line):
		break

	names.append(line)



records = []

for n in names:
	rec_conf = { "name" : n}
	records.append(rec_conf)


config = {"channels" : records}

open(".bseread_conf","w").write(json.dumps(config))

os.system("/usr/local/epics/base/bin/SL6-x86_64/caput -S PC7920-BSREAD:CONFIGURATION $(cat .bseread_conf )")
