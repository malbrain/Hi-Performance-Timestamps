#ifndef _TIMESTAMPS_H_
#define _TIMESTAMPS_H_

#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <process.h>
#include <intrin.h>

#define	 aligned_malloc _aligned_malloc
#else
#include <pthread.h>
#include <sched.h>
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

typedef union {
  struct { uint64_t low, hi; };
  struct { uint64_t base, tod[1]; };
#ifdef _WIN32
  uint64_t bits[2];
#else
  __int128 bits[1];
#endif
} TsEpoch;

bool pausey(int loops);
bool tsGo;

typedef enum {
  TSAvail = 0,  // initial unassigned TS slot
  TSIdle,       // assigned, nothing pending
  TSGen         // request for next TS
} TSState;

typedef union {
  uint64_t tsBits;
  struct {
    uint32_t tsSeqCnt;
    uint32_t tsEpoch;
  };
  volatile uint64_t tsCmd;
} Timestamp;

#if !defined(QUEUE) && !defined(SCAN) && !defined(ATOMIC) && !defined(ALIGN) && !defined(CLOCK) && !defined(RDTSC)
#define RDTSC
#endif

#ifdef ALIGN
Timestamp *tsCache;
#endif

#ifdef QUEUE

// circular queue of requests
#define TSQUEUE 12

uint64_t volatile tsHead;
uint64_t tsTail;

Timestamp *tsQueue[TSQUEUE];
#endif

//  API

void timestampInit(Timestamp *tsArray, int tsMaxClients);
Timestamp *timestampClnt(Timestamp *tsArray);
bool timestampServer(Timestamp *tsArray);
void timestampQuit(Timestamp *timestamp);
uint64_t timestampNext(Timestamp *timestamp);
#endif
