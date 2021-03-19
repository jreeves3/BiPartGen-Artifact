#!/bin/sh

for enc in direct sinz; do
  for n in $(seq 4 2 16); do
    ../bipartgen -g chess -e $enc -n $n -f "Chess${enc}N${n}.cnf"

    # To block clauses, add the size to block up to
    # ../bipartgen -g chess -e $enc -n $n -b 3 -f "Chess${enc}B3N${n}.cnf"
  done
done
