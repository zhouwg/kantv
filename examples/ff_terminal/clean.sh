#!/usr/bin/env bash

echo "remove all *.o *.a *.so out in `pwd`"

find ./ -name "*.o" -print | xargs -iXX rm -rf XX

find ./ -name "*.a" -print | xargs -iXX rm -rf XX

find ./ -name "*.so" -print | xargs -iXX rm -rf XX

find ./ -name "out" -print | xargs -iXX rm -rf XX
