#!/usr/bin/env bash

echo "remove all *.o *.a *.wasm *.so out in `pwd`"

find ./ -name "*.o" -print | xargs -iXX rm -rf XX

find ./ -name "*.a" -print | xargs -iXX rm -rf XX

find ./ -name "*.wasm" -print | xargs -iXX rm -rf XX

find ./ -name "*.so" -print | xargs -iXX rm -rf XX

find ./ -name "out" -print | xargs -iXX rm -rf XX

rm -f ffmpeg ffprobe ffmpeg_g  ffprobe_g ffplay ffplay_g

