# BiPartGen
Generate bipartite graphs and the associated CNF formulas

## Options
```bash
General Options
-g [chess|pigeon|random]       Type of graph to generate.
-n [Int]                       Size of chess NxN, number of holes in PHP N, or number of nodes in random graph.
-e [direct|split|sinz|mixed]   At Most 1 encoding, mixed uses random encoding for each node independently.
-f [FNAME]                     Filename to write cnf formula in dimacs format.
-s [Int]                       Seed for random number generator.
-M                             At Most 1 encoding applied also to both partitions.
-L                             At Least 1 encoding applied also to both partitions.
-E [Int]                       Number of edges in random graph.

Mchess Additional Options
-C [NORMAL|TORUS|CYLINDER]  Type of mutilated chessboard.

Random Graph Additional Options
-D [Float<1]       Number of edges in random graph bound by density (#edges/#possible edges).
-c [Int]           Difference in number of nodes between partitions.

PGBDD Variants
-p                 Bucket and variable ordering for Sinz encoding
-o                 Variable ordering for either Sinz or split encoding

Symmetry-Breaking Clauses
-b ...

```

scripts: scripts to generate formulas.


data: Excel spreadsheet with data labeled by Figure in the paper.


## Example
```bash
# Mutilated chessboard with 
> ./bipartgen -g chess -f chess4 -n 4 -e direct

# Pigeonhole with 8 holes, 9 pigeons, with exactly one constraints for each node using sinz
> ./bipartgen -g pigeon -f pigeon8 -n 8 -e sinz -M -L

# Random graph with 15 edges using split encoding
> ./bipartgen -g random -f random -n 6 -e split -E 15

```
## Running PGBDD Variants
```bash
# Bucket permutation and variable ordering generated for PGBDD
> ./bipartgen -g random -f randomPGBDD -n 6 -e sinz -E 15 -p
# generates randomPGBDD.cnf, randomPGBDD_bucket.order, randomPGBDD_variable.order

> python pgbdd/prototype/solver.py -i randomPGBDD.cnf -B randomPGBDD_bucket.order -p randomPGBDD_variable.order


# Variable ordering generated for PGBDD
> ./bipartgen -g random -f randomPGBDD -n 6 -e sinz -E 15 -o
# generates randomPGBDD.cnf, randomPGBDD_variable.order

# -b for default bucket elimination 
> python pgbdd/prototype/solver.py -i randomPGBDD.cnf -p randomPGBDD_variable.order -b
```
