#!/bin/sh

log=./run10.log
runs=10

# cleanup first
am slay k >/dev/null
echo -n "" > $log

for ((i=0; i<runs; i++)) do
  echo -n "Run $i: " | tee -a $log
  am run pagerank-frompmem.c0.bin | grep "Time used" | tee -a $log
done

# cat $log
echo -n "Average: "
awk '{ total += $5; count++ } END { print total/count " seconds."}' $log
