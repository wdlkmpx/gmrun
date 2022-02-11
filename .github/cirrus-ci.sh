#!/bin/sh

if [ "$1" = "freebsd" ] ; then
	#export REPO_AUTOUPDATE=NO
	pkg install -y devel/pkgconf x11-toolkits/gtk20
	exit $?
fi

if [ "$1" = "macos" ] ; then
	#export HOMEBREW_NO_AUTO_UPDATE=1
	# pkg-config is already installed
	brew install gtk+3
	exit $?
fi

# ============================================

uname -a

./configure \
	&& make \
	&& make install
exit_code=$?

for i in config.h config.log config.mk
do
	echo "
===============================
	$i
===============================
"
	cat ${i}
done

exit ${exit_code}
