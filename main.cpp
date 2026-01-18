
#include <iostream>
#include <chrono>
#include <cstdint>
#include <thread>
#include <deque>

using namespace std;
using namespace std::chrono;

// System architecture: monotonic time, dt clamp, fixed-step accumulation, interpolation, load control & stability.
// Three-layer design: real-time measurement, simulation time, presentation time.

const double MAX_DT_SECONDS = 0.05;              // Typical real-time systems use 10-50ms. (dt clamping)
const double FIXED_DT_SECONDS = 0.01;            // Simulation tick. Deterministic, predictable, testable. (fixed step accumulation)
const int MAX_SIMULATION_STEPS_PER_FRAME = 5;    // Hard safety cap. Prevents infinite catch-up if system lags. (load control & stability)
// Without this: lag -> more steps -> more CPU -> more lag -> death spiral.
// With this: simulation is bounded, CPU is capped, system degrades gracefully.

const size_t MAX_COMMAND_QUEUE_SIZE = 32;        // Hard upper bound for input pressure. Prevents unbounded memory growth.

enum class CommandType {                         // Represents "intent" coming from UI, network or sensors.
    Accelerate, Stop
};

struct Command {                                 // Small, copyable, time-agnostic instruction. Safe to queue or batch.
    CommandType type;
    double value;                                // Parameter for the command (acceleration magnitude).
};

std::deque<Command> commandQueue;                // Chosen for stable pointers, fast push/pop, and good cache behavior.

int64_t nowMs() {                                // Always use int64_t for time: explicit width, overflow-safe.
    return chrono::duration_cast<chrono::milliseconds>(
               chrono::steady_clock::now().time_since_epoch()
               ).count();
    // 1. Returns a monotonic, relative millisecond timestamp (not wall-clock time).
    // 2. steady_clock never goes backwards and is not affected by system time changes.
    // 3. time_since_epoch() is relative to an arbitrary start point; only differences are meaningful.
    // 4. duration_cast makes time unit conversion explicit and prevents hidden precision loss.
    // 5. count() extracts the raw integer value of the duration.
    // Suitable for simulation ticks, scheduling, and causal ordering in real-time systems.
}

struct SystemState {
    double position;                             // Continuous state variable; example of a physical property.
    double velocity;                             // Rate of change of position; essential for integration.
    bool valid;                                  // Data validity flag; simulation stops evolving when false.
};

void updateSystem(SystemState& state, double dtSeconds) {
    if (!state.valid) return;                    // Invalid systems don't evolve.
    state.position += state.velocity * dtSeconds; // Integrate position.

    if (state.position < 0.0) {                  // Prevent physically impossible negative position.
        state.position = 0.0;
        state.velocity = 0.0;
        state.valid = false;                     // Mark state invalid; logical failure protection.
    }
}

bool enqueueCommand(const Command& cmd) {        // UI/Input boundary. Can be called anytime; does not touch simulation state.
    if (commandQueue.size() >= MAX_COMMAND_QUEUE_SIZE) {
        return false;                            // If queue is full, drop command (Overload protection policy).
    }
    commandQueue.push_back(cmd);                 // Add command to queue.
    return true;
}

void applyCommand(SystemState& state, const Command& cmd) {
    if (!state.valid) return;                    // Invalid systems do not accept commands.
    switch(cmd.type) {
    case CommandType::Accelerate:                // Adjust velocity, not position. Physics integration happens in updateSystem().
        state.velocity += cmd.value;
        break;
    case CommandType::Stop:                      // Immediate velocity cancellation. Deterministic in fixed-step context.
        state.velocity = 0.0;
        break;
    }
}

SystemState interpolateState(const SystemState& prev, const SystemState& curr, double alpha) {
    SystemState out = curr;                      // Interpolating function for display layer.
    out.position = prev.position * (1.0 - alpha) + curr.position * alpha;
    out.velocity = prev.velocity * (1.0 - alpha) + curr.velocity * alpha;
    return out;
}

int main() {
    SystemState currentState {0.0, 1.0, true};
    SystemState previousState;
    int64_t lastTickMs = nowMs();
    double timeAccumulator = 0.0;                // Buffer for unprocessed real time. Prevents time loss and instability.

    while (true) {                               // Infinite loop: continuous operation like C2 or sensor processing loops.

        // --- LAYER 1: TEMPORAL MEASUREMENTS (INPUT LAYER) ---
        int64_t now = nowMs();                   // Sample time once per loop.
        int64_t dtMs = now - lastTickMs;         // Elapsed time since last loop; drives physics and scheduling.
        lastTickMs = now;                        // Update temporal anchor to prevent dt accumulation errors.
        double dtSeconds = dtMs / 1000.0;        // Convert to seconds (canonical unit for physics evolution).

        // --- LAYER 2: SECURITY GATE (CLAMPING) ---
        if (dtSeconds > MAX_DT_SECONDS) {        // Protect simulation from exploding if real time jumps.
            dtSeconds = MAX_DT_SECONDS;          // This is the clamp; throw away excess real time.
        }
        timeAccumulator += dtSeconds;            // Track total usable time (Measurement != Simulation).

        int stepsThisFrame = 0;

        // --- LAYER 3: DETERMINISTIC ENGINE (SIMULATION LAYER) ---
        while (timeAccumulator >= FIXED_DT_SECONDS && stepsThisFrame < MAX_SIMULATION_STEPS_PER_FRAME) {
            previousState = currentState;        // Backup state before update to allow for interpolation.

            const int MAX_COMMANDS_PER_STEP = 4; // Limit commands per step to prevent physics starvation.
            int processed = 0;
            while (!commandQueue.empty() && processed < MAX_COMMANDS_PER_STEP) {
                applyCommand(currentState, commandQueue.front()); // Process commands deterministically (FIFO).
                commandQueue.pop_front();
                processed++;
            }

            updateSystem(currentState, FIXED_DT_SECONDS); // Source of truth; advances physics in fixed 10ms slices.
            timeAccumulator -= FIXED_DT_SECONDS;          // Spend the simulated time.
            stepsThisFrame++;
        }

        if (stepsThisFrame == MAX_SIMULATION_STEPS_PER_FRAME) {
            timeAccumulator = 0.0;               // If overloaded, discard excess time to prevent spiral-of-death.
        }

        for (int i = 0; i < 10; ++i) {           // Simulated UI/Input burst; does not belong to simulation layer.
            enqueueCommand(Command{CommandType::Accelerate, 0.1});
        }

        // --- LAYER 4: PRESENTATION LAYER ---
        double alpha = timeAccumulator / FIXED_DT_SECONDS; // Calculate fractional progress between ticks.
        SystemState visualState = interpolateState(previousState, currentState, alpha); // Blend states for smooth visuals.

        cout << "t =" << now << "ms dt=" << dtMs << " pos=" << visualState.position
             << " vel=" << visualState.velocity << " valid=" << visualState.valid << endl;

        this_thread::sleep_for(milliseconds(16)); // Limits update rate to prevent CPU hogging; introduces controlled latency.

        // Note on Stalls: If loop stalls (debugger/OS scheduling), dt becomes large.
        // Without clamping, a "time step explosion" occurs, breaking stability, causality, and safety.
        // Principle: Real time is measured continuously, but state must advance in controlled quanta.
    }

    return 0;
}

// Fixed step accumulation: canonical solution for engines and simulators.
// Logic: Accumulate real time, consume in fixed slices.
// Result: Simulation is stable, deterministic, and frame-rate independent.
