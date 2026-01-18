# C2 Deterministic Simulation Engine

A high-performance C++ core designed for **Command and Control (C2)** contexts. This project implements a deterministic, fixed-timestep simulation loop engineered for mission-critical stability, ensuring that physical state evolution remains consistent regardless of hardware performance or temporal jitter.

## 🛠 Technical Highlights

* **Fixed-Step Accumulation:** Implements the canonical "Fix Your Timestep" pattern. By decoupling real-time measurement from simulation logic (using a `timeAccumulator`), the system guarantees that physics and state updates occur in exact `10ms` quanta, ensuring predictable and testable behavior.
* **Temporal Safety & Load Control:** Features a multi-layered defense against "Spiral of Death" scenarios. By utilizing `MAX_SIMULATION_STEPS_PER_FRAME` and `dt` clamping, the engine gracefully degrades under CPU stress rather than allowing latency to accumulate infinitely.
* **State Interpolation:** Separates the **Simulation Layer** from the **Presentation Layer**. The engine calculates a fractional `alpha` to blend the previous and current states, providing jitter-free visual output while maintaining a strictly deterministic backend.
* **Bounded Input Pressure:** Implements a command-queue pattern with a hard upper bound (`MAX_COMMAND_QUEUE_SIZE`). This protects the simulation from memory exhaustion and "command storms" often encountered in high-frequency sensor fusion or UI-heavy environments.



## 📡 Operational Features

### 1. Temporal Integrity
* **Monotonic Time Measurement:** Utilizes `std::chrono::steady_clock` to ensure time never flows backward, a critical requirement for causal ordering in tactical systems.
* **Overflow-Safe Math:** Uses explicit `int64_t` widths for millisecond timestamps to prevent overflow errors in long-running deployments.

### 2. Resilience & Reliability
* **Graceful Degradation:** When the system cannot keep up with real-time demands, it prioritizes **Causality** and **CPU Survival** by dropping excess time, preventing the simulation from "exploding" or becoming unresponsive.
* **Invalid State Protection:** Employs a `valid` flag logic; if physical constraints are violated (e.g., negative position), the system halts evolution for that entity to prevent error propagation.

## 🏗 System Architecture

The core is modularized into three distinct temporal layers of responsibility:
* **Measurement Layer:** Interfaces with the OS to capture real passage of time (`nowMs`).
* **Simulation Layer (Source of Truth):** Processes `CommandType` intents and evolves `SystemState` in fixed slices.
* **Presentation Layer:** Interpolates data for display/logging without mutating the core simulation state.



## 🚀 Getting Started

### Prerequisites
* C++17 compatible compiler (GCC, Clang, or MSVC)
* Qt Creator (optional, `.pro` file provided)

### Build & Run
1. **Clone the repository:** `git clone https://github.com/sebastianvento/Deterministic-Real-Time-Simulation-Engine.git`
2. Open `DeterministicReal-TimeSimulationEngine.pro` in Qt Creator.
3. Ensure the build directory is configured correctly for console output.
4. Build and run using: `qmake && make`

### Build Environment
* **Framework:** Pure C++ / Qt 6.x (for build tooling)
* **OS:** macOS
* **Compiler:** Apple Clang (C++17)
* **Build Tool:** qmake / make
