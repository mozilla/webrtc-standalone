#!/bin/sh
if [ "$GECKO_BIN_DIR" = "" ] ; then
  GECKO_BIN_DIR=/Volumes/fennec/gecko-desktop/obj-x86_64-apple-darwin12.5.0
fi
if [ "$TESTAPP_PATH" = "" ] ; then
  export TESTAPP_DIR=$GECKO_BIN_DIR/media/webrtc/signaling/test
fi
if [ "$GECKO_DIST_BIN" = "" ] ; then
  GECKO_DIST_BIN=$GECKO_BIN_DIR/dist/bin
fi
DEBUG=0
if [ "$1" = "-d" ] ; then
  DEBUG=10
fi
DYLD_LIBRARY_PATH=$GECKO_DIST_BIN NSPR_LOG_MODULES=signaling:$DEBUG python ../serve.py
