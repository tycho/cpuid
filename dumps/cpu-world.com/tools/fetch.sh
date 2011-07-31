#!/bin/bash
INDEX=${1}

# Translated version
[ -f ${INDEX}.txt ] && exit 0

# Skip marker
grep -E "^${INDEX}$" .fetchignore > /dev/null
[ $? -eq 0 ] && exit 0
[ -f ${INDEX}.skip ] && exit 0

# Unprocessed version
[ -f ${INDEX} ] && exit 0

echo "Attempting to fetch missing dump ${INDEX}"
# Fetch from the target
curl -s --retry 5 -o ${INDEX} "http://www.cpu-world.com/cgi-bin/CPUID.pl?CPUID=${INDEX}&RAW_DATA=1"

# Make sure it actually has content.
grep -E '(is not public|Cannot find (CPUID data|public CPUID))' ${INDEX} > /dev/null
if [ $? -eq 0 ]; then
	# No content. Mark it as skippable.
	rm -f ${INDEX}
	touch ${INDEX}.skip
fi

# Translate it to CPUID's native dump format.
if [ -f ${INDEX} ]; then
	tools/translate.py ${INDEX}
	if [ -f ${INDEX}.translated ]; then
		../../cpuid -d -f ${INDEX}.translated > ${INDEX}.txt
		rm ${INDEX} ${INDEX}.translated
	fi
else
	[ ! -f ${INDEX}.skip ] && echo "WARNING: Something went wrong with ${INDEX}"
fi

[ -f ${INDEX}.txt ] && echo "Success: ${INDEX}"

exit 0
