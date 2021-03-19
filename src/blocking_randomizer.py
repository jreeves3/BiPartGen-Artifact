# @file blocking_randomizer.py
#
# @usage python blocking_randomizer.py <input.cnf> <prob> <num_randoms> [seed]
#
# @author Cayden Codel (ccodel@andrew.cmu.edu)
#
# @bug No known bugs.
#

import random
import sys
import time

if len(sys.argv) < 4 or len(sys.argv) > 5:
    print("Must supply four or five arguments")
    exit()

input_cnf = sys.argv[1]
prob = float(sys.argv[2])
num_trials = int(sys.argv[3])
seed = int(time.time() * 1000)
if len(sys.argv) == 5:
    seed = int(sys.argv[4])

random.seed(a=seed)

# Open file and separate the lines
f = open(input_cnf, "r")
lines = f.readlines()
f.close()
index_of_comment = [idx for idx, s in enumerate(lines) if s[0] == 'c' and 'matchings' in s][0]

cnf_vars = int(lines[0].split(" ")[2])
cnf_clauses = int(lines[0].split(" ")[3])
normal_lines = lines[1:index_of_comment]
comment = lines[index_of_comment]
rest = lines[(index_of_comment + 1):]
breaking_clauses = len(rest)
breaking_goal = int(float(breaking_clauses) * prob)
removed = breaking_clauses - breaking_goal

for i in range(1, (num_trials + 1)):
    random.shuffle(rest)
    random_rest = rest[:breaking_goal]

    f = open("Randomizer{0}.cnf".format(i), "w+")
    f.write("p cnf {0} {1}\n".format(cnf_vars, cnf_clauses - removed))
    for line in normal_lines:
        f.write(line)
    f.write(comment)
    for line in random_rest:
        f.write(line)
    f.close()
