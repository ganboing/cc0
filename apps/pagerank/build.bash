#!/bin/bash

function compile() {
    file=$1
    processed="strsplitted-$file"
    bin="${file}.bin"
    log="${file}.log"
    ./libi0/strsplitter.bash $file >$processed
    echo "cc0 $processed -o $bin >$log"
    cc0 $processed -o $bin -g >$log
    rm -f $processed
}

compile pagerank-stdin2pmem.c0
compile pagerank-frompmem.c0
