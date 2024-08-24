#!/usr/bin/env bash

PQDSFLAGS=`pkg-config --cflags --libs parquet arrow-dataset`
LDFLAGS="-Wl,-rpath=`pkg-config --libs-only-L arrow | cut -c 3-`"

g++ streaming_engine.cpp -O0 -g -o streaming_engine $PQDSFLAGS $LDFLAGS
