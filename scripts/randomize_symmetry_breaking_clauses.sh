#!/bin/sh

# Range of n values to pull from
for n in $(seq 4 1 10); do
  # Encodings here
  for enc in sinz linear; do

    # Probabilities here
    for p in 0.50; do

      # Usage: <name_of_cnf> <prob> <num_seeds> [optional_seed]
      python ../src/blocking_randomizer.py Pigeon${enc}B2N${n}.cnf $p 60

      # Will output as Randomizer${i}.cnf -> conveniently rename and move
      for i in $(seq 1 1 60); do
        mv ./Randomizer${i}.cnf Pigeon${enc}B2N${n}P${p}Seed${i}.cnf
      done

    done
  done
done
