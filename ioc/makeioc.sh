#!/bin/bash

# Cleanup
rm bin/*
rm dbd/*

# Copy relevant files
cp ../src/O.3.14.12_SL5-x86/libBSREAD*.so bin/
cp ../src/O.3.14.12_SL5-x86/BSREAD.dbd dbd/
