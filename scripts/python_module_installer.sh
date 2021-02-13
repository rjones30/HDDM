#!/bin/bash
echo looking for modules in $1
for mod in `ls $1/hddm_*.so`; do
    module=$(echo $mod | sed 's/.*\///' | sed 's/\..*//')
    if nm $mod | grep -q "PyInit_$module"; then
        echo found python3 module $mod
        mkdir -p $2/python3
        cp $mod $2/python3/
    else
        echo found python2 module $mod
        mkdir -p $2/python2
        cp $mod $2/python2/
    fi
done
