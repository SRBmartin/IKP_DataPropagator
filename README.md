# IKP Project Documentation — Propagator Early-Warning System

### 1. Introduction
## Problem statement

We implement a distributed early-warning service for natural disasters (floods, fires, storms). Services form a tree topology; data always flows from the root to leaves. The root represents the state; destinations represent regions or sub-regions. Messages can be sent to all or selected parts of the country.

## Goals

* Generate warnings at a Data Source.
* Propagate them via Central Propagators along the tree to the appropriate leaves.
* Collect and display received warnings at Data Destinations.
* Validate correctness and performance with small and large test sets (≈ 10,000 events) and document results.
* Provide leak-free operation (memory & handle), graceful shutdown, and clean code organization.

### 2. Design

![Diagram](https://i.ibb.co/6R89hK1d/diagram-export-9-30-2025-9-26-50-PM.png)

## High-level architecture

* Common (library): shared data structures & utilities (hash map, queues, node loader, wire protocol helpers, connection pooling, warning model, debug utilities).
* CentralPropagator (CP): a node that receives warnings and routes them towards the correct subtree (or terminates them if current node is the hop/leaf).
* DataDestination (DD): a leaf process that accepts incoming warnings and counts/logs them.
* DataSource (DS): orchestrator that loads the tree topology (from nodes.csv), launches all CP/DD processes, generates warnings, and sends them into the tree.

## Process lifecycle & orchestration

Launcher (DS/cp_launcher.c) starts all CPs and DDs via `CreateProcessA`, assigns them to a Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` to ensure group termination on exit, and keeps `PROCESS_INFORMATION` handles for coordination.

DS announces shutdown by setting named events (`Global\CP_Exit_<id>` / `Global\DD_Exit_<id>`) so each process exits gracefully. CP & DD register `SetConsoleCtrlHandler` to translate console signals into those events and complete cleanups.

## Network & protocol

* **TCP** sockets (`WinSock2`). Each sender uses a connection pool (`hash-map` keyed by `address:port`) to reuse sockets and avoid connect overhead.
* Structure for each Warning (**big-endian** lengths/values):

    1. uint32 city length, bytes of UTF-8 city
    2. uint32 destination node id length, bytes
    3. uint32 warning type
    4. uint64 value bits (network order)
    5. uint64 timestamp (epoch seconds, network order)

* CP/DD deserialize by reading lengths & fields with a `recv_all` loop until the full message is obtained.

## Routing logic

* `nodes.csv` defines the tree. At startup CP builds an in-memory map of its subtree.

* On receive:
    * If destination is me → terminate locally.
    * Else ascend from the destination upwards until the immediate child under this CP is found; forward to that next hop.

## Concurrency model

* CP runs a listener thread (accept + message read) and dispatches Warning handling to a ThreadPool (`tp_create`, `tp_submit`, `tp_shutdown`) backed by a `TSQueue`.
* DD runs two threads: a TCP listener that enqueues warnings and a processor thread that dequeues, logs, and increments an atomic counter.
* DS runs generator and network-sender threads, sharing a `TSQueue` for back-pressure friendly decoupling.

### 3. Data structures

## HashMap (chaining, resizable)
* Separate chaining via forward-linked lists; pluggable hash, key-equality, and optional free callbacks (keys/values).
* Used for:
    * All nodes by id during `node_load_all`,
    * CP subtree map,
    * Connection pool keyed by (`address, port`).

## Queue & TSQueue

* Queue: singly-linked FIFO with optional item destructor.
* TSQueue: thread-safe wrapper with `CRITICAL_SECTION` + condition variables for blocking producer/consumer; allows bounded capacity when needed for back-pressure.

## Node model (NodeInfo)

* Fields: id, address, port, parent, children[], child_count, and derived type (`ROOT`, `CENTRAL`, `DESTINATION`).
* Built by parsing `nodes.csv`, connecting parents/children, and classifying node type from adjacency.

## Warning model

* Fields: city, type, value, timestamp, dest_node.
* Helpers: `warning_to_string` for logging & `warning_destroy` for cleanup.

## Connection pool

* Key: (address, port).
* Value: `SOCKET*`, allowing close & free in a single destructor.
* Protected by a global critical section to ensure thread-safe get/create/close.

### 4. Test results

* We executed a single fixed workload of 10,000 warnings to satisfy the “large dataset” requirement. This section documents setup, method, and evidence for correctness, no memory/handle leaks, and graceful shutdown.

### 4.1 Setup
* Workload: exactly 10,000 generated warnings (pseudo-random city/type/value/destination).
* Topology: tree defined in Common/nodes.csv.
* Builds: Debug (with CRT leak reports) and Release.
* Processes under observation: Data Source, Central Propagator, and Data Destination.

### 4.2 Method

* Counters tracked (PerfMon) per process:

    * Process → Handle Count (handle leak detection)
    * Process → Private Bytes (native heap usage)
    * % Processor Time (workload activity window)

* Snapshots (Visual Studio):

    * Periodic Heap snapshots to observe allocation/heap-size deltas across the run.

* Run protocol:

    * Start DS → wait until CP/DD are listening.
    * Fire the 10,000-event generator (continuous).
    * Let the system go idle after completion; continue recording counters.
    * Initiate graceful shutdown (DS signals all processes).
    * Collect CRT leak logs (`MemoryLeakReports/memory_leaks_<proc>.txt`) in Debug.

### 4.3 Evidence

* **Perf monitor**
    * Handle counts **rise** during startup and **plateau** during steady-state/idle (no monotonic growth).
    * Private Bytes stabilize after the warm-up; no unbounded increase during idle.

![Perf Monitor](https://i.ibb.co/4gRLxTZr/perf.webp)

* **Heap snapshots**
    * Deltas show typical growth during initialization, then decrease..

![Profiler](https://i.ibb.co/Cp5KX53Q/profiler.webp)

* **CRT leak logs (Debug)**
    * Each process writes a final report; runs finished with “No memory leaks detected.”

### 5. Conclusion

* Stress testing at ≈10k events shows stable handles and memory, and graceful shutdown, meeting the evaluation criteria.

* **Correctness**: Every warning generated by the Data Source reached the intended Data Destination.
* **Memory safety**: Visual Studio heap snapshots showed typical initialization growth followed by a net decrease at the end of the run. Debug CRT reports confirm that all application allocations were released on shutdown with no leaks detected. A representative report for the Belgrade node states:
```
=== MEMORY DEBUG REPORT FOR Belgrade ===
Log initialized at program start
=== FINAL MEMORY ANALYSIS ===
[INFO] No memory leaks detected.

[STATISTICS] Initial memory state
Total blocks: 0
Total bytes: 0
CRT blocks: 72
CRT bytes: 16831
Free blocks: 0
Free bytes: 0

[STATISTICS] Final memory state
Total blocks: 0
Total bytes: 0
CRT blocks: 141
CRT bytes: 34069
Free blocks: 0
Free bytes: 0

[STATISTICS] Memory allocation difference
Total blocks: 0
Total bytes: 0
CRT blocks: 69
CRT bytes: 17238
Free blocks: 0
Free bytes: 0

[SYSTEM] Process memory usage: 5292 KB
```
* **Graceful shutdown**: listeners exited, queues were drained, worker threads joined, sockets and handles were closed, and the Data Destination wrote its final count file. The `Job Object` provided a safety net to guarantee cleanup of any child processes.

### 6. Potential improvements

* **Back-pressure end-to-end**: dynamically adjust generator pace by monitoring DD queue depth or CP backlog.
* **Retry & transient fault handling**: exponential backoff and circuit-breaker for connection issues.
* **Topology reload**: hot-swap nodes.csv (versioned) to add/remove regions without restart.