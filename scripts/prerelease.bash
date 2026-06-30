#!/bin/bash
set +x
cd examples
for i in *
do
  cd $i
  ../../scripts/generate_platform_h.py
  cd ..
done
cd .. # Frugal-IoT
pwd
scripts/generate_keywords.py
