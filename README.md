# TCP Congestion Control Analysis with ns-3

This project simulates and analyzes different TCP congestion control algorithms using **ns-3**.

The goal is to compare the behavior of major TCP variants under different network loads and topologies. The project evaluates **TCP NewReno**, **TCP CUBIC**, and **TCP Vegas** using metrics such as throughput, delay, packet loss, congestion window, goodput, and Jain's fairness index.

---

## Authors

- Amirkasra Ahmadi
- Shamim Rahimi

---

## Project Overview

Congestion control is one of the most important mechanisms in TCP. It determines how a sender adjusts its transmission rate based on network conditions such as delay, packet loss, and available bandwidth.

This project studies the performance of three TCP congestion control algorithms:

- **TCP NewReno**
- **TCP CUBIC**
- **TCP Vegas**

The simulations are implemented in **ns-3** using C++ and analyzed using Python scripts.

---

## Main Goals

The main objectives of this project are:

- Simulate TCP congestion control algorithms in ns-3
- Compare TCP NewReno, CUBIC, and Vegas
- Analyze performance under different traffic loads
- Study the impact of network topology on TCP behavior
- Generate plots and summary tables for comparison
- Evaluate throughput, delay, loss rate, cwnd, goodput, and fairness

---

## Technologies Used

- C++
- Python
- ns-3
- FlowMonitor
- NetAnim
- Matplotlib
- Pandas
- Seaborn
- NumPy

---

## Project Structure

```text
.
├── phase1/
│   ├── project3-tcp.cc
│   ├── visualize_results.py
│   ├── summary.csv
│   └── plots/
│
├── phase2/
│   ├── tcp_phase2.cc
│   ├── analyze_phase2.py
│   ├── phase2_summary.csv
│   └── plots_phase2/
│
├── ACN-Project3-401105956-401170507.pdf
└── README.md
```

---

## Phase 1: Dumbbell Topology

In the first phase, a fixed **Dumbbell topology** is used.

The topology includes:

- 3 sender nodes
- 2 intermediate routers
- 3 receiver nodes
- One bottleneck link between the routers

The bottleneck link is configured with:

```text
Bandwidth: 10 Mbps
Delay: 50 ms
Queue size: 100 packets
```

The access links are configured with:

```text
Bandwidth: 100 Mbps
Delay: 20 ms
```

---

## Phase 1 Test Scenarios

The simulations are performed with different numbers of flows:

```text
5 flows
20 flows
50 flows
```

And different packet rates:

```text
100 packets/second
300 packets/second
1000 packets/second
```

Each scenario is tested with:

```text
TCP NewReno
TCP CUBIC
TCP Vegas
```

---

## Phase 2: Multiple Topologies

In the second phase, the effect of network topology on TCP performance is studied.

The implemented topologies are:

```text
Dumbbell
Bus
Tree
```

For each topology, the same TCP variants are compared:

```text
TCP NewReno
TCP CUBIC
TCP Vegas
```

The goal of this phase is to understand how topology structure affects congestion, delay, packet loss, throughput, and fairness.

---

## Performance Metrics

The project evaluates TCP performance using the following metrics:

### Throughput

Measures the average data transfer rate of each flow.

### Delay

Measures the average end-to-end delay experienced by packets.

### Packet Loss Rate

Shows the percentage of packets lost during transmission.

### Congestion Window

Tracks the behavior of TCP's congestion window over time.

### Goodput

Measures useful successfully delivered application-level data.

### Jain's Fairness Index

Measures how fairly bandwidth is shared among competing flows.

---

## How to Run the Simulations

### 1. Copy the C++ Simulation Files to ns-3

Copy the phase 1 simulation file into the ns-3 `scratch` directory:

```bash
cp phase1/project3-tcp.cc /path/to/ns-3/scratch/
```

Copy the phase 2 simulation file:

```bash
cp phase2/tcp_phase2.cc /path/to/ns-3/scratch/
```

Then go to the ns-3 root directory:

```bash
cd /path/to/ns-3
```

---

## Running Phase 1

### Run the Full Phase 1 Test Matrix

```bash
./ns3 run "scratch/project3-tcp --runAll=true --outputDir=results"
```

This runs all combinations of:

- TCP variants
- Flow counts
- Packet rates

The final summary is saved in:

```text
results/summary.csv
```

---

### Run a Single Phase 1 Scenario

```bash
./ns3 run "scratch/project3-tcp --runAll=false --tcpType=ns3::TcpCubic --nFlows=20 --packetRate=300 --outputDir=results"
```

Example TCP types:

```text
ns3::TcpNewReno
ns3::TcpCubic
ns3::TcpVegas
```

---

## Analyzing Phase 1 Results

After running the simulations, use the Python analysis script:

```bash
python3 phase1/visualize_results.py --results-dir results
```

This script generates plots for:

- Throughput comparison
- Delay comparison
- Packet loss comparison
- Fairness comparison
- Goodput comparison
- Congestion window traces
- Scalability analysis
- Multi-metric comparison

Generated plots are saved in:

```text
results/plots/
```

---

## Running Phase 2

### Run the Full Phase 2 Test Matrix

```bash
./ns3 run "scratch/tcp_phase2 --runAll=true --outputDir=results_phase2"
```

This runs simulations for:

- Dumbbell topology
- Bus topology
- Tree topology
- TCP NewReno
- TCP CUBIC
- TCP Vegas

The final summary is saved in:

```text
results_phase2/phase2_summary.csv
```

---

### Run a Single Phase 2 Scenario

Example for Dumbbell topology:

```bash
./ns3 run "scratch/tcp_phase2 --topology=dumbbell --tcpType=Vegas --nNodes=5 --nFlows=40 --packetRate=300 --outputDir=results_phase2"
```

Example for Bus topology:

```bash
./ns3 run "scratch/tcp_phase2 --topology=bus --tcpType=CUBIC --nNodes=8 --nFlows=40 --packetRate=300 --outputDir=results_phase2"
```

Example for Tree topology:

```bash
./ns3 run "scratch/tcp_phase2 --topology=tree --tcpType=NewReno --treeDepth=3 --nFlows=40 --packetRate=300 --outputDir=results_phase2"
```

Available topology options:

```text
dumbbell
bus
tree
all
```

Available TCP options:

```text
NewReno
CUBIC
Vegas
all
```

---

## Analyzing Phase 2 Results

After running phase 2 simulations:

```bash
python3 phase2/analyze_phase2.py --results-dir results_phase2
```

This script generates:

- Topology comparison plots
- TCP comparison plots
- Heatmaps
- Goodput comparison
- Congestion window plots
- Radar charts

Generated plots are saved in:

```text
results_phase2/plots/
```

---

## Example Results

In phase 1, the simulations show different behavior for each TCP variant:

- **TCP CUBIC** usually achieves high throughput but may create higher packet loss under heavy congestion.
- **TCP NewReno** behaves more conservatively than CUBIC.
- **TCP Vegas** is delay-based and usually produces lower delay and lower packet loss by reacting before the queue becomes full.

In phase 2, topology has a visible impact on performance:

- **Tree topology** can provide better goodput because of its more distributed structure.
- **Bus topology** may increase delay and packet loss because packets pass through multiple intermediate queues.
- **Dumbbell topology** is useful for analyzing bottleneck behavior.

---

## Output Files

The simulations generate several types of output files:

### CSV Summary Files

```text
phase1/summary.csv
phase2/phase2_summary.csv
```

### Plot Files

```text
phase1/plots/
phase2/plots_phase2/
```

### Congestion Window Traces

```text
cwnd-flow-*.dat
```

### NetAnim Files

```text
*.xml
```

These files can be used for visualizing network behavior and analyzing TCP congestion control dynamics.

---

## Key Features

- ns-3 based TCP congestion control simulation
- Custom rate-controlled traffic generator
- Dumbbell topology implementation
- Bus and Tree topology implementation
- TCP NewReno, CUBIC, and Vegas comparison
- FlowMonitor-based metric extraction
- Congestion window tracing
- CSV result generation
- Python-based visualization
- Heatmaps, radar charts, and comparison plots
- Fairness analysis using Jain's fairness index

---

## Educational Purpose

This project was developed for an Advanced Computer Networks course.

It demonstrates practical concepts such as:

- TCP congestion control
- ns-3 simulation design
- Bottleneck link behavior
- Queue overflow and packet loss
- Delay-based vs loss-based congestion control
- Network topology impact on transport-layer performance
- Traffic analysis and visualization

---

## License

This project is created for educational purposes.
