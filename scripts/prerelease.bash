#!/bin/bash
set +x
cd examples
# Remove PlatformIO build dirs: `pio package publish` walks examples/ including .pio/libdeps,
# which hold nested copies of this library, and fails on the resulting >255-char example names.
rm -rf */.pio
for i in *
do
  cd $i
  ../../scripts/generate_platform_h.py
  cd ..
done
cd .. # Frugal-IoT
pwd
scripts/generate_keywords.py
