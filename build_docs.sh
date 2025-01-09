#!/bin/bash

rm -rf docs
mkdir -p docs/doxygen

doxygen

cd twl-docs
mkdocs build
cd ..

cp -r twl-docs/site/* docs/
