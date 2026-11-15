#!/bin/bash
LD_LIBRARY_PATH=$PWD/lib/lib64 \
  LD_PRELOAD=$PWD/lib/lib64/libruntime.so \
  ./out/app $@
