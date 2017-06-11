#!/bin/sh

#Get all required m4 macros required for configure
libtoolize -ci
aclocal

#generate configure
autoconf

#Generate config.h.in
autoheader

#Generate Makefile.in's
automake -ac
