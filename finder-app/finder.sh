#!/bin/sh
if [ $# -ge 2 ]
then
    if [ -d $1 ]
    then
	num_files=$(find $1 -type f | wc -l)
	num_match=$(cat $1/* | grep $2 | wc -l)
	echo "The number of files are $num_files and the number of matching lines are $num_match"
    else
	echo "No such folder exists"
	exit 1
    fi
else
    echo "Argument counts are less than expected"
    exit 1
fi
