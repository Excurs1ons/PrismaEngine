#pragma once

// ═════════════════════════════════════════════════════════════════════════════
// PrismaEngine Profiling Macros
//
// Usage:
//   PROFILE_SCOPE("MyScope")     — RAII scope timer
//   PROFILE_FUNCTION()            — auto-named scope (__FUNCTION__)
//   PROFILE_FRAME_MARKER()        — marks a frame boundary
//
// Compile-time removal:
//   Define PRISMA_PROFILING_ENABLED=0 before including this header (or via
//   CMake compile definition) to turn all macros into no-ops.
// ═════════════════════════════════════════════════════════════════════════════

#include "ProfilerCPU.h"

// ── helpers ─────────────────────────────────────────────────────────────────
#define PRISMA_PROFILE_CONCAT_IMPL(a, b) a##b
#define PRISMA_PROFILE_CONCAT(a, b)      PRISMA_PROFILE_CONCAT_IMPL(a, b)
#define PRISMA_PROFILE_UNIQUE(name)      PRISMA_PROFILE_CONCAT(name, __LINE__)

// ── compile-time switch ─────────────────────────────────────────────────────
#if !defined(PRISMA_PROFILING_ENABLED) || PRISMA_PROFILING_ENABLED

/// Profile a named scope.  Starts a timer on entry, stops on exit.
/// The sample is pushed into the thread-local CpuProfiler.
#define PROFILE_SCOPE(name) \
    ::Prisma::Profiling::ScopedTimer PRISMA_PROFILE_UNIQUE(_prProfile)(name)

/// Shorthand for PROFILE_SCOPE(__FUNCTION__).
#define PROFILE_FUNCTION() \
    PROFILE_SCOPE(__FUNCTION__)

/// Insert a frame boundary marker into the thread-local CpuProfiler.
#define PROFILE_FRAME_MARKER() \
    ::Prisma::Profiling::CpuProfiler::FrameMark()

#else

// ── disabled – all macros expand to nothing ─────────────────────────────────
#define PROFILE_SCOPE(name)
#define PROFILE_FUNCTION()
#define PROFILE_FRAME_MARKER()

#endif // PRISMA_PROFILING_ENABLED
