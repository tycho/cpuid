#!/bin/bash

echo "Reprocessing: ${1}"
../../cpuid -d -f ${1} > ${1}.tmp
mv ${1}.tmp ${1}

exit 0