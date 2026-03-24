# C2 Deterministic Simulation Engine

A C++ implementation of a Command and Control (C2) simulation core. This project uses a fixed-timestep loop to ensure consistent state evolution across different hardware performance levels and frame rates.

## 🛠 Technical Implementation

* **Fixed-Step Accumulation:** Implements a `timeAccumulator` to decouple real-time measurement from simulation logic. Updates occur in constant `10ms` slices, ensuring deterministic behavior.
* **Update Constraints:** Prevents execution lag (the "Spiral of Death") by using `MAX_SIMULATION_STEPS_PER_FRAME`. This clamps the number of updates per frame to maintain system responsiveness under CPU load.
* **State Interpolation:** Implements a fractional `alpha` calculation to blend previous and current states, allowing for smooth visual or log output without mutating the deterministic backend.
* **Command Queue Management:** Uses a bounded command queue (`MAX_COMMAND_QUEUE_SIZE`) to manage input pressure and prevent memory exhaustion during high-frequency data ingestion.

## 📡 Logic & Reliability

### 1. Temporal Handling
* **Steady Clock:** Utilizes `std::chrono::steady_clock` for monotonic time measurement.
* **Timestamp Precision:** Uses `int64_t` for millisecond tracking to prevent overflow during long-running execution.

### 2. Error Handling
* **Delta Clamping:** Excess time is dropped if the simulation cannot maintain real-time parity, prioritizing system stability over historical catch-up.
* **State Validation:** Includes logic to halt entity evolution if physical constraints (e.g., coordinate bounds) are violated, preventing error propagation.

## 🏗 System Architecture

The engine is divided into three functional layers:
* **Measurement:** Interfaces with the system clock to capture real-time deltas.
* **Simulation (Domain):** Processes `CommandType` inputs and evolves the `SystemState` in fixed intervals.
* **Presentation:** Handles data output/logging and state interpolation for the user interface.
