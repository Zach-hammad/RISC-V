# RISC-V CPU Design

## Overview

This project presents an advanced implementation of a RISC-V CPU, evolving from a single-cycle architecture to a fully pipelined design. It incorporates sophisticated features such as branch prediction, cache management, out-of-order execution, and a PC-signature hit predictor to enhance performance and efficiency.

## Features

- **Branch Prediction**: Implements dynamic branch prediction mechanisms to minimize control hazards and improve instruction flow.
- **Cache Management**: Integrates instruction and data caches to reduce memory access latency and increase throughput.
- **Out-of-Order Execution**: Allows instructions to be executed as resources become available, rather than strictly adhering to program order, to optimize CPU utilization.
- **PC-Signature Hit Predictor**: Utilizes a signature-based approach to predict instruction hits, further reducing pipeline stalls.

## Architecture Evolution

- **Single-Cycle Implementation**: Initial design executing one instruction per clock cycle, serving as a foundation for understanding basic CPU operations.
- **Multi-Cycle Implementation**: Introduced multiple cycles per instruction to handle more complex operations and improve resource utilization.
- **Pipelined Architecture**: Final design features a fully pipelined CPU with stages for Fetch, Decode, Execute, Memory Access, and Write Back, enabling instruction-level parallelism and higher instruction throughput.

## Technologies Used

- **Hardware Description Language**: Verilog
- **Simulation Tools**: ModelSim for functional verification and timing analysis
- **Version Control**: Git for tracking changes and collaboration

## Learning Outcomes

- Gained in-depth understanding of CPU architecture and RISC-V instruction sets.
- Developed skills in implementing complex features like branch prediction and out-of-order execution.
- Enhanced proficiency in hardware description languages and simulation tools.

## Challenges Faced

- Ensuring accurate timing and synchronization in a pipelined architecture.
- Debugging intricate issues related to cache coherence and branch mispredictions.
- Managing the complexity of out-of-order execution and maintaining instruction correctness.

## Getting Started

To simulate the CPU:

1. Clone the repository:
   ```bash
   git clone https://github.com/Zach-hammad/RISC-V.git
