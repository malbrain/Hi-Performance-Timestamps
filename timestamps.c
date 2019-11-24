//  Hi Performance timestamp generator.
//  Multi-process/multi-threaded clients
//  one centralized server per machine with
//  a mmap common data structure shared by 
//  processes

//  Clients are given one of the client array slots of timestamps to communicate with the server.
//  Client Requests for the next timestamp are made from and delivered into the assigned
//  client array slot.

//  The server uses slot 0 to store the last timestamp assigned as the basis for the next request.
//  The server also stores the configuration parameters in local variables.

#include "timestamps.h"
#include <stdio.h>

//  store the initialization values

uint64_t rdtscEpochs = 0;
Timestamp *tsArray;
int tsClientMax;
bool tsGo = true;

#ifdef _WIN32
__declspec(align(16)) volatile TsEpoch rdtscEpoch[1];
#else
volatile TsEpoch rdtscEpoch[1] __attribute__((__aligned__(16)));
#endif

#ifndef _WIN32
#include <x86intrin.h>
#else
#include <intrin.h>
#endif

//	atomic install 64 bit value

static bool atomicCAS64(volatile uint64_t *dest, uint64_t *comp, uint64_t *value) {
#ifdef _WIN32
  return _InterlockedCompareExchange64(dest, *value, *comp) == *comp;
}
#else
  return __atomic_compare_exchange(dest, comp, value, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED );
}
#endif

//	atomic 64 bit increment

static uint64_t atomicINC64(volatile uint64_t *dest) {
#ifdef _WIN32
  return _InterlockedIncrement64(dest);
#else
  return __sync_add_and_fetch(dest, 1);
#endif
}

//	atomic 128 bit compare and swap

bool atomicCAS128(volatile TsEpoch *where, TsEpoch *comp, TsEpoch *repl) {
#ifdef _WIN32
  return _InterlockedCompareExchange128(where->bitsX2, repl->hi, repl->low, comp->bitsX2);
}
#else
  return __atomic_compare_exchange(where->bits, comp->bits, repl->bits, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}
#endif

//  routine to wait

bool pausey(int loops) {
  if (loops < 20) return tsGo;

  pausex();
  return tsGo;
}

  // calculate and install current epoch

bool tsCalcEpoch(Timestamp *tsBase) {
time_t tod[1];

  if(time(tod) <= tsBase->tsEpoch)
    return false;
    
  tsBase->tsEpoch = (uint32_t)*tod;
  tsBase->tsSeqCnt = 1;
  return true;
}

// scan/queue

bool tsScanReq(Timestamp *tsBase) {
bool result = false;

#ifdef QUEUE
int loops = 0;

  do {
    uint64_t tsNext = tsTail + 1;

	while (tsHead == tsTail)
      if (!pausey(++loops))
		  return false;

    while (tsQueue[tsNext % TSQUEUE] == NULL)
      if (!pausey(++loops))
		  return false;

	tsQueue[tsNext % TSQUEUE]->tsBits = ++tsBase->tsBits;
    tsQueue[tsNext % TSQUEUE] = NULL;
    tsTail = tsNext;

    result = true;
  } while (++loops < 1000 || loops < 1000000 && tsHead != tsTail );
  
  return result;
#endif
#ifdef SCAN
  int idx = 0;

  while (++idx < tsClientMax)
	if( tsBase[idx].tsBits > 0 )
	  tsBase[idx].tsBits = ++tsBase->tsBits;
#endif
  return true;
}

//  API functions

bool timestampServer(Timestamp *tsBase) {
  int loops = 0;
  bool result;
  
  do {
    result = tsCalcEpoch(tsBase);

    if ((result = tsScanReq(tsBase))) continue;

	if (!pausey(++loops)) return false;
  } while (tsGo);
  return true;
}

//	tsMaxClients is the number of client slots plus one for slot zero

void timestampInit(Timestamp *tsBase, int tsMaxClients) {
#ifdef RDTSC
struct timespec spec[1];
#endif
  tsClientMax = tsMaxClients;
  tsBase->tsBits = 0;
  tsCalcEpoch(tsBase);
  tsArray = tsBase;
#ifdef RDTSC
  timespec_get(spec, TIME_UTC);
  rdtscEpoch->tod[0] = spec->tv_sec;
  rdtscEpoch->base = __rdtsc() - (1000000000 - spec->tv_nsec);
#endif
#ifdef ALIGN
  tsCache = (Timestamp *)aligned_malloc(64 * tsClientMax, 64);
#endif
}

//  Client request for tsBase slot

Timestamp *timestampClnt(Timestamp *tsBase) {
int idx = 0;
uint64_t tsAvail[1] = { TSAvail };
#ifdef SCAN
uint64_t tsCMD[1] = { TSGen };
#else
uint64_t tsCMD[1] = { TSIdle };
#endif

while (++idx < tsClientMax)
	if( tsBase[idx].tsCmd == TSAvail )
	  if (atomicCAS64(&tsBase[idx].tsCmd, tsAvail, tsCMD)) 
		  return tsBase + idx;

  return NULL;
}

//	release tsBase slot

void timestampQuit(Timestamp *timestamp) {
  timestamp->tsCmd = TSAvail;
}

//  request next timestamp

uint64_t timestampNext(Timestamp *timestamp) {
#ifdef CLOCK
  struct timespec spec[1];
#ifdef _WIN32
  timespec_get(spec, TIME_UTC);
#else
  clock_gettime(CLOCK_REALTIME, spec);
#endif
  timestamp->tsEpoch = (uint32_t)spec->tv_sec;
  timestamp->tsSeqCnt = spec->tv_nsec;
  return timestamp->tsBits;
#endif
#ifdef RDTSC
#ifdef _WIN32
__declspec(align(16)) TsEpoch newEpoch[1];
__declspec(align(16)) TsEpoch oldEpoch[1];
#else
  TsEpoch newEpoch[1] __attribute__((__aligned__(16)));
  TsEpoch oldEpoch[1] __attribute__((__aligned__(16)));
#endif
uint32_t maxRange = 1000000000;
struct timespec spec[1];
uint64_t ts, range, units;
bool once = true;
time_t tod[1];

  do {
	ts = __rdtsc();
	*tod = *(volatile time_t *)rdtscEpoch->tod;
    range = ts - rdtscEpoch->base;
    units = range / rdtscUnits;

	if (range <= rdtscUnits) 
		units = 1, printf("range underflow\n");

	// Skip down to assign Timestamp from current Epoch
	// guard against shredded load

	if (*tod != *(volatile time_t *)rdtscEpoch->tod)
        continue;

    if (units < maxRange)
	  break;

	if (once) {
      atomicINC64(&rdtscEpochs);
      once = false;
    }

    oldEpoch->base = ts - range;
    oldEpoch->tod[0] = tod[0];

    timespec_get(spec, TIME_UTC);

	// same epoch?

	if (spec->tv_sec == *tod)
      maxRange = 2000000000;
    else
      maxRange = 3000000000;

    newEpoch->base = ts - (maxRange - spec->tv_nsec);
    newEpoch->tod[0] = spec->tv_sec;

	//  Release new Epoch via atomicCAS128

    atomicCAS128(rdtscEpoch, oldEpoch, newEpoch);

  } while (true);

  // emit assigned Timestamp.

  timestamp->tsEpoch = (uint32_t)*tod;
  timestamp->tsSeqCnt = (uint32_t)units;
  return timestamp->tsBits;
#endif
#ifdef ATOMIC
	return atomicINC64(&tsArray->tsBits);
#endif
#ifdef ALIGN
    return atomicINC64(&tsCache[8 * (timestamp - tsArray)].tsBits);
#endif
#ifdef QUEUE
  uint64_t tsNext;
  int loops = 0;

  timestamp->tsBits = TSGen;
  tsNext = atomicINC64(&tsHead);
  tsQueue[tsNext % TSQUEUE] = timestamp;

  while( timestamp->tsCmd == TSGen)
    if (!pausey(++loops)) return 0;

  return timestamp->tsBits;
#endif
#ifdef SCAN
  uint64_t tsNext;
  int loops = 0;

  if( (tsNext = timestamp->tsBits) == TSGen )
    while( (tsNext = timestamp->tsCmd) == TSGen )
      if (!pausey(++loops)) return 0;

  timestamp->tsBits = TSGen;
  return tsNext;
  #endif
}
