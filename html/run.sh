#!/bin/sh
DEBUG=0
if [ "$1" = "-d" ] ; then
  DEBUG=10
fi
DYLD_LIBRARY_PATH=/Volumes/fennec/gecko-desktop/obj-x86_64-apple-darwin12.5.0/dist/bin/ NSPR_LOG_MODULES=signaling:$DEBUG python ../serve.py
