#!/bin/bash
while [ $# -gt 1 ]; do
    export $1
    echo $1
    shift
done
if [ "$(which python2)" != "" ]; then
    python2 $1
else
    python $1
fi
if [ "$(which python3)" != "" ]; then
    python3 $1
fi
