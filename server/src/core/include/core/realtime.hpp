#ifndef CORE__REALTIME_HPP
#define CORE__REALTIME_HPP

namespace beatled::core {

// Real-time scheduling helpers. On Linux these wrap the POSIX real-time
// primitives that keep audio/beat-tracking latency deterministic; on every
// other platform they compile to harmless no-ops so the call sites stay
// portable (macOS dev builds, the POSIX simulator, CI, etc.).

// Pin all current and future process memory with mlockall(MCL_CURRENT |
// MCL_FUTURE) so a page fault never stalls the real-time threads. Call once at
// startup, before the audio loop spins up. Returns true on success; logs a
// warning and returns false if the lock was refused (e.g. missing
// CAP_IPC_LOCK / RLIMIT_MEMLOCK) — a soft failure, the server still runs.
bool lock_process_memory();

// Promote the calling thread to SCHED_FIFO at the given priority so the OS
// scheduler never preempts it in favour of normal time-sharing work. Call from
// inside the thread that needs it (e.g. the beat-detector loop). Returns true
// on success; logs a warning and returns false if the policy was refused
// (insufficient privileges) — the thread keeps running under SCHED_OTHER.
bool set_thread_realtime_priority(int priority);

} // namespace beatled::core

#endif // CORE__REALTIME_HPP
