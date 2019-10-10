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
#endif

typedef union {
  uint64_t tsBits;
  struct {
    uint32_t tsSeqCnt;
    uint32_t tsEpoch;
  };
} Timestamp;

typedef union {
  uint8_t align[64];
  struct {
    Timestamp timestamp;
    int tsState;
    int tsIdx;
  };
} TSClient;

//  API

void timestampInit(int tsMaxClients);
TSClient *timestampClnt();
void timestampQuit(TSClient *tsClient);
uint64_t timestampNext(TSClient *tsClient);
