#!/bin/bash

if [ ! -x ../../cpuid ]; then
	echo "Compile CPUID first, it'll be needed for translating the data."
	exit 1
fi

if [ `ls *.txt 2>/dev/null | wc -l` -lt 1 ]; then
	echo "Fetching snapshot from Amazon S3..."
	curl -s "https://s3.amazonaws.com/neunon/cpu-world-dumps.tar.bz2" | tar xjf -
fi

LIMIT=`curl -s "http://www.cpu-world.com/cgi-bin/CPUID.pl" | grep '<td><a href="/cgi-bin/CPUID.pl?CPUID=' | perl -pe 's/^.*<td><a href="\/cgi-bin\/CPUID\.pl\?CPUID=([0-9]+)".*$/$1/g' | head -n 1`
[ -z "$LIMIT" ] && exit 1

# Create an associative array of the indices to avoid
declare -A fetchignore
for i in $(cat .fetchignore); do
	fetchignore[${i}]=1
done

echo "Attempting to fetch through record ${LIMIT}..."
for i in $(seq 1 $LIMIT); do
	[ -f ${i}.txt ] && continue;
	[ -f ${i}.skip ] && continue;
	[ ! -z ${fetchignore[${i}]} ] && continue
	echo "Adding ${i} to queue..." >&2
	echo ${i}
done | parallel tools/fetch.sh {}

echo "Pruning duplicate dumps..."
tools/prune_duplicates.py

echo "Updating .fetchignore file..."
for a in $(ls *.skip 2>/dev/null); do
	echo ${a%.skip} >> .fetchignore
	rm ${a}
done
sort -n .fetchignore > .fetchignore.tmp
mv .fetchignore.tmp .fetchignore
