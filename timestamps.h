#ifndef _TIMESTAMPS_H_
#define _TIMESTAMPS_H_

#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <intrin.h>
#include <process.h>
#include <time.h>
#include <winbase.h>
#include <windows.h>

#define aligned_malloc _aligned_malloc
#else
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <x86intrin.h>
#endif

#ifndef _WIN32
#ifdef apple
#include <libkern/OSAtomic.h>
#define pausex() OSMemoryBarrier()
#else
#define pausex() sched_yield()
#endif
#else
#define pausex() YieldProcessor()
#endif

#if !defined(ALIGN) && !defined(ATOMIC) && !defined(CLOCK) && !defined(RDTSC)
#define RDTSC
#endif

typedef union {
  struct {
    uint64_t low;
    uint64_t hi;
  };

  struct {
    uint64_t base;
    time_t tod[1];
  };

#ifdef _WIN32
  uint64_t bitsX2[2];
#else
  __int128 bits[1];
#endif
} TsEpoch;

uint64_t rdtscUnits;

bool pausey(int loops);
bool tsGo;

typedef enum {
  TSAvail = 0,  // initial unassigned TS slot
  TSIdle,       // assigned, nothing pending
  TSGen         // request for next TS
} TSCmd;

typedef union {
  uint64_t tsBits[2];
  struct {
    uint16_t tsIdx;
    uint16_t tsCmd;
    uint32_t tsSeqCnt;
    time_t tsEpoch;
  };
#ifdef ALIGN
  uint8_t filler[64];
#endif
} Timestamp;

//  API

int timestampCmp(Timestamp* ts1, Timestamp* ts2);
void timestampInit(Timestamp* tsArray, int tsMaxClients);
uint16_t timestampClnt(Timestamp* tsArray, int tsMaxClients);
void timestampQuit(Timestamp* tsArray, uint16_t idx);
void timestampNext(Timestamp* tsArray, uint16_t idx);
#endif
