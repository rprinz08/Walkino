#!/bin/bash

DIST=./walkino-legacy.zip

rm -f $DIST

zip -r $DIST \
    ./readme.md \
    ./misc/ \
    ./walkino-remove-legacy.sh \
    ./examples/Walkino/ \
    ./hardware/walkera/ \
    ./hardware/tools/walkera \
	> /dev/null

zip -T $DIST
