#!/bin/sh
# This is free and unencumbered software released into the public domain.
#  For more information, please refer to <https://unlicense.org>

# ./data/zmtrace.sh

if ! test -f autogen.sh ; then
	exit
fi

if ! test -f configure ; then
	./autogen.sh;
fi

export MALLOC_TRACE=$(pwd)/mtrace.txt

if ! test -f src/gmrun ; then
	./configure --prefix=/usr --enable-mtrace;
	make;
fi

./src/gmrun;

mtrace=
for i in $(echo $PATH | tr ':' ' ')
do
	if test -x $i/mtrace ; then
		mtrace=$i/mtrace;
		break;
	fi
done

if test "x${mtrace}" != "x" ; then
	mtrace src/gmrun mtrace.txt > mtrace-out.txt;
	printf "see mtrace-out.txt\n";
else
	printf "mtrace is missing\n";
	printf "see mtrace.txt\n"
fi
