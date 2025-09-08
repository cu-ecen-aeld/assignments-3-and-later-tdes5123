#!/bin/sh

filesdir="$1"
searchstr="$2"

if [ "$#" -lt 2 ]; then
	echo "Error: Wrong number of arguments."
	exit 1
fi

if [ ! -d "$filesdir" ]; then
	echo "Error: Bad arguments."
	exit 1
fi

num_files=`grep -rl "$searchstr" "$filesdir" | wc -l`
num_lines=`grep -r "$searchstr" "$filesdir" | wc -l` 

echo "The number of files are $num_files and the number of matching lines are $num_lines"
