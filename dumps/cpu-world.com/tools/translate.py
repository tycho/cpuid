#!/usr/bin/env python

import fileinput
import re
import sys

match_leaf = re.compile("(^<div class='cf_desc_h'>CPUID.+?<\/div>$.+?^<\/div>$)", re.MULTILINE | re.DOTALL)
match_leaf_input = re.compile("^.+?>CPUID (?P<eax_in>[0-9A-Fa-f]+_[0-9A-Fa-f]+)<\/div>$", re.MULTILINE | re.DOTALL)
match_leaf_input_ext = re.compile("^.+?>CPUID (?P<eax_in>[0-9A-Fa-f]+_[0-9A-Fa-f]+) [(]ECX=(?P<ecx_in>[0-9A-Fa-f]+)[)]<\/div>$", re.MULTILINE | re.DOTALL)
match_leaf_output = re.compile("^(E[A-D]X): ([0-9A-Fa-f]+)", re.MULTILINE | re.DOTALL);

def extract(file, infilename):
	#print "Translating %s..." % infilename
	f = open(infilename + ".translated", "wb")
	for leaf in match_leaf.finditer(file):
		leaf = leaf.groups()[0]
		
		# Hash out the input line
		input = match_leaf_input.match(leaf)
		if not input:
			input = match_leaf_input_ext.match(leaf)
		if not input:
			print "%s: Couldn't match the input line." % infilename
			sys.exit(1)
		input = input.groupdict()
		
		# Now each output line
		outputs = dict()
		for output in match_leaf_output.finditer(leaf):
			outputs[output.groups()[0]] = output.groups()[1]
		if len(outputs) == 0:
			print "%s: Couldn't match any output lines." % infilename
			sys.exit(1)

		eax = int(input['eax_in'].replace("_", ""), 16)
		prefix = ""
		if 'ecx_in' in input:
			prefix = "CPUID %08x, index %d = " % (
				eax,
				int(input['ecx_in'], 16) )
		else:
			prefix = "CPUID %08x, results = " % eax

		f.write(prefix + "%08x %08x %08x %08x\n" % (
			int(outputs['EAX'], 16),
			int(outputs['EBX'], 16),
			int(outputs['ECX'], 16),
			int(outputs['EDX'], 16) ))
	f.close()
	
if __name__ == "__main__":
	whole = ""
	prevfile = None
	for line in fileinput.input():
		if prevfile == None:
			prevfile = fileinput.filename()
		if fileinput.isfirstline() and len(whole) != 0:
			extract(whole, prevfile)
			prevfile = fileinput.filename()
			whole = ""
		whole += line

	if len(whole) != 0:
		extract(whole, prevfile)
