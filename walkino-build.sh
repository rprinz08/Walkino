#!/bin/bash

DIST=./walkino.zip


rm -f $DIST

zip -r $DIST .vscode/ \
    readme.md \
    misc/ \
    walkino-build.cmd walkino-build.sh \
    walkino-remove.cmd walkino-remove.sh \
    examples/Walkino/ \
    hardware/walkera/ \
    hardware/tools/walkera/

zip -T $DIST
