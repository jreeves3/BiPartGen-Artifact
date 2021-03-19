# BiPartGen
Generate bipartite graphs and the associated CNF formulas

## Options
```bash
General Options
-g [chess|pigeon|random]       Type of graph to generate.
-n [Int]                       Size of smaller partition in graph, or nxn board for chess.
-e [direct|linear|sinz|mixed]  At-Most-One encoding, mixed randomly selects encoding for each node.
-f [FNAME]                     Filename to write cnf formula in dimacs format.
-s [Int]                       Seed for random number generator.
-M                             At-Most-One encoding applied also to both partitions.
-L                             At-Least-One encoding applied also to both partitions.
-E [Int]                       Number of edges in random graph.
-v                             Verbose (display density of generated bipartite graph).

Random Graph Additional Options
-D [Float<1]       Number of edges in random graph bound by density (#edges/#possible edges).
-c [Int]           Difference in number of nodes between partitions.

PGBDD Variants
-p                 Bucket and variable ordering for Sinz encoding (FNAME_bucket.order, FNAME_variable.order).
-o                 Variable ordering for either Sinz or linear encoding (FNAME_variable.order).

Symmetry-Breaking Clauses
-b [Int]                       Add symmetry-breaking clauses to disallow perfect matchings of up to this size.

```

## scripts
Scripts to generate a subset of benchmark formulas.
* random - random graphs with 130 edges, n from [11,20], encodings from [direct,sinz,linear,mixed], -A (default) and -B (Exactly-One) constraints
* randomPGBDD - random graphs with 130 edges, n from [11,20], encodings from [sinz,linear], -A (default) constraints, bucket permutation (-Sched) and variable ordering (-Ord) options. (Note: this outputs ..\_variable.order, ..\_bucket.order files with usecase shown in the example section below)
* gen\_chess.sh - Generates mutilated chessboard CNFs of varying sizes on direct, Sinz encodings.
* gen\_pigeon.sh - Generates pigeon CNFs of varying sizes on direct, Sinz, linear encodings
* randomize\_symmetry\_breaking\_claues.sh - Runs symmetry-broken CNFs into a randomizer to select a subset.

## data 
Excel spreadsheets (Random Experiments, Symmetry-Breaking Experiments) with sheets labeled by Figure.

## Solvers
* [Kissat](https://github.com/arminbiere/kissat)
* [Lingeling](https://github.com/arminbiere/lingeling)
* [SaDiCaL](http://fmv.jku.at/sadical)
* [PGBDD](https://github.com/rebryant/pgbdd)

## Example
```bash
# Mutilated chessboard nxn with direct encoding
> ./bipartgen -g chess -f chess4 -n 4 -e direct

# Pigeonhole with 8 holes, 9 pigeons, with exactly one constraints for each node using sinz
> ./bipartgen -g pigeon -f pigeon8 -n 8 -e sinz -M -L

# Random graph with 15 edges using linear encoding
> ./bipartgen -g random -f random -n 6 -e linear -E 15

# Mutilated chessboard instance with K22, 6-cycle, and K33s symmetry-breaking clauses added
> ./bipartgen -g chess -f sb_chess8 -n 8 -e direct -b 3

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
