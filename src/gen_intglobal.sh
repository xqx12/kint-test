#!/bin/bash

g++ -c -o Annotation.o Annotation.cc Annotation.h -I/home/xqx/projects/git/kint/src/  `llvm-config --ldflags` -lLLVM-`llvm-config --version` `llvm-config --cxxflags`
