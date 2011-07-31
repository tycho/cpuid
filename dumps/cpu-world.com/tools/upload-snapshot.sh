#!/bin/bash

FILENAME=cpu-world-dumps.tar.bz2

echo "Creating tarball..."
tar cf - *.txt .fetchignore | bzip2 -9c > $FILENAME

echo "Uploading to Amazon S3..."
s3cmd put $FILENAME s3://neunon/
s3cmd setacl -P s3://neunon/cpu-world-dumps.tar.bz2
