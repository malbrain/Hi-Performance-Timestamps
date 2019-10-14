#ifndef _TIMESTAMPS_H_
#define _TIMESTAMPS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <process.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

#ifndef _WIN32
#ifdef apple
#include <libkern/OSAtomic.h>
#define pause() OSMemoryBarrier()
#else
#define pause() sched_yield()
#endif
#else
#define pause() YieldProcessor()
#endif

typedef enum {
  TSAvail = 0,      // initial unassigned TS slot
  TSIdle,           // assigned, nothing pending
  TSGen             // request for next TS
} TSState;

typedef union {
  uint64_t tsBits;
  struct {
    uint32_t tsSeqCnt;
    uint32_t tsEpoch;
  };
  struct {
    uint32_t fill1;
    uint16_t fill2;
    uint16_t tsCmd;
  };
} Timestamp;

//  API

void timestampInit(Timestamp *tsArray, int tsMaxClients);
Timestamp *timestampClnt(Timestamp *tsArray);
uint64_t timestampServer(Timestamp *tsArray);
void timestampQuit(Timestamp *timestamp);
uint64_t timestampNext(Timestamp *timestamp);
#endif