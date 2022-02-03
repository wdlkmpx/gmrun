#!/bin/sh
# This is free and unencumbered software released into the public domain.
# For more information, please refer to <http://unlicense.org/>

# make target dir1 [dir2 ...]

make=$1
target=$2
shift
shift

pwd=$(pwd)

for dir in ${@}
do
	echo "${make}: Entering directory '${pwd}/${dir}'"
	if ! [ -e "$dir" ] ; then
		echo "ERROR: '$dir' directory does not exist"
		exit 1
	fi
	if ! [ -d "$dir" ] ; then
		echo "ERROR: '$dir' is not a directory"
		exit 1
	fi
	#--
	cd "$dir"
	if ! ${make} ${target} ; then
		exit 1
	fi
	#--
	cd "${pwd}"
	echo "${make}: Leaving directory '${pwd}/${dir}'"
done

exit 0
