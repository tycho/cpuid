#!/bin/bash

FILENAME=cpu-world-dumps.tar.bz2

T=$(which s3cmd 2>/dev/null)
if [ "x$T" == "x" ]; then
	echo "Please install s3cmd, it's needed for the upload."
	exit 1
fi

echo "Creating tarball..."
tar cf - *.txt .fetchignore | bzip2 -9c > $FILENAME

echo "Uploading to Amazon S3..."
s3cmd put $FILENAME s3://neunon/
s3cmd setacl -P s3://neunon/cpu-world-dumps.tar.bz2

rm $FILENAME
