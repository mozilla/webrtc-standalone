#!/bin/sh

PATH_FILE=../.config

if [ ! -f $PATH_FILE ] ; then
  echo $PATH_FILE not found!
  exit 1
fi

# strip out the parenthesis from the Makefile config so it can be used by sh
eval `tr -d '()' < $PATH_FILE`

# Setup for both Linux and Mac to run player
export LD_LIBRARY_PATH=$GECKO_OBJ/dist/lib
export DYLD_LIBRARY_PATH=$GECKO_OBJ/dist/lib
export NSPR_LOG_MODULES=all:5

# Start simulator
python simulator.py
