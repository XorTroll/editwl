#!/bin/bash

rm -rf docs
mkdir -p docs/doxygen

doxygen

cd editwl-docs
mkdocs build
cd ..

cp -r editwl-docs/site/* docs/
