# @file cnf_shuffler.py
#
# @usage python cnf_shuffler.py [-cnsv] <input.cnf>
# 
# @author Cayden Codel (ccodel@andrew.cmu.edu)
#
# @bug No known bugs.

import random
import sys
import os
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-c", "--clauses", dest="clauses", action="store_true",
                    help="Shuffle the order of the clause lines in the CNF")
parser.add_option("-n", "--names", dest="names", action="store_true",
                    help="Shuffle the names of the literals in the clauses")
parser.add_option("-r", "--random", dest="seed",
                    help="Provide a randomization seed")
parser.add_option("-s", "--signs", dest="signs",
                    help="Switch the sign of literals with the provided prob")
parser.add_option("-v", "--variables", dest="variables",
                    help="Shuffle the order of the variables with prob")

(options, args) = parser.parse_args()
f_name = sys.argv[-1]

if len(sys.argv) == 1:
    print("Must supply a CNF file")
    exit()

# Parse the provided CNF file
if not os.path.exists(f_name) or os.path.isdir(f_name):
    print("Supplied CNF file does not exist or is directory", file=sys.stderr)
    exit()

cnf_file = open(f_name, "r")
cnf_lines = cnf_file.readlines()
cnf_file.close()

# Verify that the file has at least one line
if len(cnf_lines) == 0:
    print("Supplied CNF file is empty", file=sys.stderr)
    exit()

# Do treatment on the lines
cnf_lines = list(map(lambda x: x.strip(), cnf_lines))

# Verify that the file is a CNF file
header_line = cnf_lines[0].split(" ")
if header_line[0] != "p" or header_line[1] != "cnf":
    print("Supplied file doesn't follow DIMACS CNF convention")
    exit()

num_vars = int(header_line[2])
num_clauses = int(header_line[3])

print(" ".join(header_line))
cnf_lines = cnf_lines[1:]

# If the -r option is specified, initialize the random library
if options.seed is not None:
    random.seed(a=int(options.seed))
else:
    random.seed()

# If the -c option is specified, permute all other lines
if options.clauses:
    cnf_lines = random.shuffle(cnf_lines)

# If the -v option is specified, permute the order of variables
if options.variables is not None:
    var_prob = float(options.variables)
    if var_prob <= 0 or var_prob > 1:
        print("Prob for var shuffling not between 0 and 1", file=sys.stderr)
        exit()

    # TODO this doesn't work if each line is a single variable, etc.
    for i in range(0, len(cnf_lines)):
        line = cnf_lines[i]
        atoms = line.split(" ")
        if atoms[0][0] == "c" or random.random() > var_prob:
            continue

        if atoms[-1] == "0":
            atoms = atoms[:-1]
            random.shuffle(atoms)
            atoms.append("0")
        else:
            random.shuffle(atoms)
        cnf_lines[i] = " ".join(atoms)

# Now do one pass through all other lines to get the variable names
if options.names:
    literals = {}

    for line in cnf_lines:
        if line[0] == "c":
            continue

        atoms = line.split(" ")
        for atom in atoms:
            lit = abs(int(atom))
            if lit != 0:
                literals[lit] = True

    # After storing all the literals, permute
    literal_keys = list(literals.keys())
    p_keys = list(literals.keys())
    random.shuffle(p_keys)
    zipped = list(zip(literal_keys, p_keys))
    for k, p in zipped:
        literals[k] = p

    for i in range(0, len(cnf_lines)):
        line = cnf_lines[i]
        if line[0] == "c":
            continue

        atoms = line.split(" ")
        for j in range(0, len(atoms)):
            if atoms[j] != "0":
                if int(atoms[j]) < 0:
                    atoms[j] = "-" + str(literals[abs(int(atoms[j]))])
                else:
                    atoms[j] = str(literals[int(atoms[j])])
        cnf_lines[i] = " ".join(atoms)

if options.signs is not None:
    signs_prob = float(options.signs)
    if signs_prob < 0 or signs_prob > 1:
        print("Sign prob must be between 0 and 1", file=sys.stderr)
        exit()

    flipped_literals = {}
    for i in range(0, len(cnf_lines)):
        line = cnf_lines[i]
        if line[0] == "c":
            continue

        # For each symbol inside, flip weighted coin and see if flip
        atoms = line.split(" ")
        for j in range(0, len(atoms)):
            atom = atoms[j]
            if atom != "0":
                if flipped_literals.get(atom) is None:
                    if random.random() <= signs_prob:
                        flipped_literals[atom] = True
                    else:
                        flipped_literals[atom] = False
                
                if flipped_literals[atom]:
                    atoms[j] = str(-int(atom))
        cnf_lines[i] = " ".join(atoms)
            
# Finally, output the transformed lines
for line in cnf_lines:
    print(line)
