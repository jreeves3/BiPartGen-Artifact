#!/bin/sh

for enc in direct sinz split; do
  for n in $(seq 4 1 16); do
    ../bipartgen -g pigeon -e $enc -n $n -f "Pigeon${enc}N${n}.cnf"

    # To block clauses, add the size to block up to
    # ../bipartgen -g pigeon -e $enc -n $n -b 2 -f "Pigeon${enc}B2N${n}.cnf"
  done
done
