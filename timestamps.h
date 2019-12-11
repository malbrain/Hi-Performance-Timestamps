#ifndef _TIMESTAMPS_H_
#define _TIMESTAMPS_H_

#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <process.h>
#include <intrin.h>
#include <time.h>

#define	 aligned_malloc _aligned_malloc
#else
#include <x86intrin.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
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
#ifndef _WIN32
  __int128 tsBits128;
#endif
#ifdef ALIGN
  uint8_t filler[64];
#endif
} Timestamp;

//  API

void timestampInstall(Timestamp *dest, Timestamp *src);
int64_t timestampCmp(Timestamp * ts1, Timestamp * ts2);
void timestampInit(Timestamp *tsArray, int tsMaxClients);
uint16_t timestampClnt(Timestamp *tsArray, int tsMaxClients);
void timestampQuit(Timestamp *tsArray, uint16_t idx);
void timestampNext(Timestamp *tsArray, uint16_t idx);
void timestampCAS(Timestamp *dest, Timestamp *src, int chk);

//	intrinsic atomic machine code

uint64_t atomicINC64(volatile uint64_t *dest);
#ifdef _WIN32
bool atomicCAS128(volatile uint64_t *what, uint64_t *comp, uint64_t *repl);
#else
bool atomicCAS128(volatile __int128 *what, __int128 *comp, __int128 *repl);
#endif
#endif
