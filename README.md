# BiPartGen
Generate bipartite graphs and the associated CNF formulas

## Options
```bash
General Options
-g [chess|pigeon|random]       Type of graph to generate.
-n [Int]                       Size of smaller partition in graph, or nxn board for chess?.
-e [direct|split|sinz|mixed]   At-Most-One encoding, mixed randomly selects encoding for each node.
-f [FNAME]                     Filename to write cnf formula in dimacs format.
-s [Int]                       Seed for random number generator.
-M                             At-Most-One encoding applied also to both partitions.
-L                             At-Least-One encoding applied also to both partitions.
-E [Int]                       Number of edges in random graph.
-v                             Verbose (display density of generated bipartite graph).

Mchess Additional Options
-C [NORMAL|TORUS|CYLINDER]  Type of mutilated chessboard.

Random Graph Additional Options
-D [Float<1]       Number of edges in random graph bound by density (#edges/#possible edges).
-c [Int]           Difference in number of nodes between partitions.

PGBDD Variants
-p                 Bucket and variable ordering for Sinz encoding (FNAME_bucket.order, FNAME_variable.order).
-o                 Variable ordering for either Sinz or split encoding (FNAME_variable.order).

Symmetry-Breaking Clauses
-b ...

```

scripts: Scripts to generate a subset of benchmark formulas.
* random - random graphs with 130 edges, n from [12,20], encodings from [direct,sinz,split,mixed], -A (default) and -B (Exactly-One) constraints
* randomPGBDD - random graphs with 130 edges, n from [12,20], encodings from [sinz,split], -A (default) constraints, bucket permutation (-Sched) and variable ordering (-Ord) options. (Note: this outputs .._variable.order, .._bucket.order files with usecase shown in the example section below)
* symmetry-breaking - 


data: Excel spreadsheets (Random Experiments, Symmetry-Breaking Experiments) with sheets labeled by Figure #.

## Solvers
* [Kissat](https://github.com/arminbiere/kissat)
* [Lingeling](https://github.com/arminbiere/lingeling)
* [SaDiCaL](http://fmv.jku.at/sadical)
* [PGBDD](https://github.com/rebryant/pgbdd)

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
