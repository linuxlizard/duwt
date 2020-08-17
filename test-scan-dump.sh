#!/bin/sh

set -eu

# enable -x while debugging tests
set -x

SD=./build/libiw/scan-dump

echo "run some simple scan-dump tests"

if [ ! -f $SD ] ; then
	echo "failure! can't find scan-dump at $SD"
	exit 1
fi

function fail {
	# not sure how to find the failed test; run with -x for now
	echo "test failure!"
	exit 1
}

function wiface {
	for f in /sys/class/net/wl* 
	do
		. $f/uevent
		# for now, take the first entry found
		break
	done
}

trap fail err

# verify can at least run
$SD 2> /dev/null  && exit 1

# verify bad interface detected
$SD dave0 2> /dev/null  && exit 1

INTERFACE=
wiface
if [ -z "$INTERFACE" ] ; then
	echo "failure: no wifi devices found"
	exit 1
fi

echo found interface=$INTERFACE

# this should succeed
$SD $INTERFACE &> /dev/null || exit 1

# -i and -o cannot be used at the same time
$SD -i /tmp/dave.dat -o /tmp/dave.dat $INTERFACE 2> /dev/null && exit 1

# -h should give us some nice help
sdhelp=$($SD -h 2>&1 && exit 1)
echo $sdhelp

echo "success!"
