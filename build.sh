#!/bin/bash

DIST=./walkera.zip


rm -f $DIST

zip -r $DIST build.cmd build.sh .vscode/ \
    examples/WalkDuino \
    hardware/walkera/ \
    hardware/tools/walkera/

zip -T $DIST
