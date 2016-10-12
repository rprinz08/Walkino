#!/bin/bash

DIST=./walkino.zip

rm -f $DIST

zip -r $DIST \
    ./readme.md \
    ./misc/ \
    ./walkino-remove.cmd ./walkino-remove.sh \
    ./examples/Walkino/ \
    ./hardware/walkera/ \
    ./hardware/tools/walkera

zip -T $DIST
