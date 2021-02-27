# How to debug Memgraph Mage query modules implemented in C/C++?

Install a required compiler.
```bash
apt install gcc-8 g++-8
```

Configure and build.
```
cmake -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_BUILD_TYPE=Debug ..
make
```

Run Memgraph container.
```bash
docker run --rm -p 7687:7687 \
    -v /home/mg/Workspace/code/mage/cpp/build/pagerank_module/:/usr/lib/memgraph/query_modules \
    -v /home/mg/Workspace/code/mage:/home/mg/Workspace/code/mage \
    --cap-add=SYS_PTRACE \
    memgraph --also-log-to-stderr --log-level=TRACE
```

Extend Memgraph container configuration.
```bash
docker exec -it -u root container_name bash
apt update && apt install -y procps gdb libpthread-stubs0-dev
```

Attach to Memgraph process with GDB.
```bash
gdb -p $(pgrep memgraph)
```

