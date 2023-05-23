#!/bin/sh

if [ "$IMAGE" = "alpine" ]
then
	if [ "x$ACT" != "x" ]
	then
		# I use local repository mirrors that need to use HTTP since they're
		# set up via DNS manipulation (and thus the TLS CN doesn't match).
		sed -ri 's/https/http/g' /etc/apk/repositories
	fi
	apk update
	apk add bash
fi

exit 0
