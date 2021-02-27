# How to Improve PageRank scalability by using C/C++ Part 3

An option is to import the [European road network
dataset](https://memgraph.com/blog/how-to-build-a-route-planning-application-with-breadth-first-search-and-dijkstras-algorithm):
```bash
echo "CREATE INDEX ON :Country(name);" | mgconsole
echo "CREATE INDEX ON :City(name);" | mgconsole
cat countries.txt | mgconsole && cat cities.txt | mgconsole && cat roads.txt | mgconsole
```

Also, Memgraph Lab v1.3 comes with dataset included (option to download and
import various datasets).

Compile commands:
```bash
cmake -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_BUILD_TYPE=Release ..
make
```

Single-threaded implementation from 2021-02-06:
```bash
CALL pagerank.run() YIELD * RETURN * LIMIT 10;
```

Multi-threaded implementation from the [mage
repo](https://github.com/memgraph/mage):
```bash
CALL cpp_pagerank_module.pagerank() YIELD * RETURN * LIMIT 10;
```
