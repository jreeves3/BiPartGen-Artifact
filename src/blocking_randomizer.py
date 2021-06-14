#/**********************************************************************************
# Copyright (c) 2021 Joseph Reeves and Cayden Codel, Carnegie Mellon University
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
# associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
# NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
# OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# **************************************************************************************************/

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
