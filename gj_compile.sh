#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

mode=$1
orig_compiler=$2
gj_dir=$3

if [ $mode = "c" ]; then
    gj_compiler="$DIR/gjcc"
else
    gj_compiler="$DIR/gjccxx"
fi

shift; shift; shift

if [ -z "$GJ" ]; then
    $orig_compiler $@
else
    $gj_compiler "--gj_dir=$gj_dir" $@
    $orig_compiler $@
fi
