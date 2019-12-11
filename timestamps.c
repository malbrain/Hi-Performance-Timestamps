//  Hi Performance timestamp generator.
//  Multi-process/multi-threaded clients
//  a mmap common data structure shared by 
//  processes

//  Clients are given one of the client array slots of timestamps.
//  Client Requests for the next timestamp are made from and delivered into the assigned
//  client array slot.

//  The some server flavors use slot 0 to store the last timestamp assigned as the basis for the next request.

#include "timestamps.h"
#include <stdio.h>

//  store the initialization values

uint64_t rdtscEpochs = 0;

#ifdef _WIN32
__declspec(align(16)) volatile TsEpoch rdtscEpoch[1];
#else
volatile TsEpoch rdtscEpoch[1] __attribute__((__aligned__(16)));
#endif

//	atomic install 64 bit value

static bool atomicCAS64(volatile uint64_t *dest, uint64_t comp, uint64_t value) {
#ifdef _WIN32
  return _InterlockedCompareExchange64(dest, value, comp) == comp;
}
#else
  return __atomic_compare_exchange(dest, comp, value, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED );
}
#endif

//	atomic install 16 bit value

static bool atomicCAS16(volatile uint16_t *dest, uint16_t comp, uint16_t value) {
#ifdef _WIN32
  return _InterlockedCompareExchange16(dest, value, comp) == comp;
}
#else
  return __atomic_compare_exchange(dest, comp, value, false, __ATOMIC_RELEASE,
                                   __ATOMIC_RELAXED);
}
#endif

//	atomic 64 bit increment

uint64_t atomicINC64(volatile uint64_t *dest) {
#ifdef _WIN32
  return _InterlockedIncrement64(dest);
#else
  return __sync_add_and_fetch(dest, 1);
#endif
}

//	atomic 128 bit compare and swap

bool atomicCASEpoch(volatile TsEpoch *what, TsEpoch *comp, TsEpoch *repl) {
  return atomicCAS128(what->bitsX2, comp->bitsX2, repl->bitsX2);
}

#ifdef _WIN32
bool atomicCAS128(volatile uint64_t * what, uint64_t * comp, uint64_t * repl) {
  return _InterlockedCompareExchange128(what, repl[1], repl[0], comp);
}
#else
bool atomicCAS128(volatile __int128 *what, __int128 *comp, __int128 *repl) {
  return __atomic_compare_exchange(what, comp, repl, false,
                                   __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}
#endif

//  routine to wait

bool pausey(int loops) {
  if (loops < 20) return tsGo;

  pausex();
  return tsGo;
}

//	tsMaxClients is the number of client slots plus one for slot zero
// for flavor ALIGN, place tsBase on 64 byte alignment

void timestampInit(Timestamp *tsBase, int tsMaxClients) {
#ifdef RDTSC
struct timespec spec[1];

  timespec_get(spec, TIME_UTC);
  rdtscEpoch->tod[0] = spec->tv_sec;
  rdtscEpoch->base = __rdtsc() - (1000000000ULL - spec->tv_nsec);
#endif
}

//  Client request for tsBase slot

uint16_t timestampClnt(Timestamp *tsBase, int maxClient) {
int idx = 0;

  while (++idx < maxClient)
	if( tsBase[idx].tsCmd == TSAvail )
	  if (atomicCAS16(&tsBase[idx].tsCmd, TSAvail, TSIdle)) 
		  return idx;

  return 0;
}

//	release tsBase slot

void timestampQuit(Timestamp *tsBase, uint16_t idx) {

  tsBase[idx].tsCmd = TSAvail;
}

//  request next timestamp

void timestampNext(Timestamp *tsBase, uint16_t idx) {
  Timestamp prev[1];

  prev[0].tsBits[0] = tsBase[idx].tsBits[0];
  prev[0].tsBits[1] = tsBase[idx].tsBits[1];
#ifdef CLOCK
  struct timespec spec[1];
  do {
#ifdef _WIN32
    timespec_get(spec, TIME_UTC);
#else
    clock_gettime(CLOCK_REALTIME, spec);
#endif
    tsBase[idx].tsEpoch = spec->tv_sec;
    tsBase[idx].tsSeqCnt = spec->tv_nsec;
    tsBase[idx].tsIdx = idx;

  } while (timestampCmp(prev, tsBase + idx));

  return;
#endif
#ifdef RDTSC
#ifdef _WIN32
  __declspec(align(16)) TsEpoch newEpoch[1];
  __declspec(align(16)) TsEpoch oldEpoch[1];
#else
  TsEpoch newEpoch[1] __attribute__((__aligned__(16)));
  TsEpoch oldEpoch[1] __attribute__((__aligned__(16)));
#endif
#if defined(WSL) || defined(_WIN32)
  uint64_t maxRange = 1000000000;
  struct timespec spec[1];
  uint64_t ts, range, units;
  bool once = true;
  time_t tod[1];

  do {
    do {
      ts = __rdtsc();
      *tod = *(volatile time_t *)rdtscEpoch->tod;
      range = ts - rdtscEpoch->base;
      units = range / rdtscUnits;

      if (range <= rdtscUnits) units = 1, printf("range underflow\n");

      // Skip down to assign Timestamp from current Epoch
      // guard against shredded load

      if (*tod != *(volatile time_t *)rdtscEpoch->tod) continue;

      if (units < maxRange) break;

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

      atomicCASEpoch(rdtscEpoch, oldEpoch, newEpoch);

    } while (true);

#elif defined(__linux__)
  uint32_t maxRange = 1000000000;
  uint64_t ts, range, units;
  time_t tod[1];
  bool once = true;

  do {
    do {
      ts = __rdtsc();
      *tod = rdtscEpoch->tod[0];
      range = ts - rdtscEpoch->base;
      units = range / rdtscUnits;

      if (time(NULL) == *tod)
        if (*tod == *(volatile time_t *)rdtscEpoch->tod && (units < maxRange))
          break;

      if (once) {
        atomicINC64(&rdtscEpochs);
        once = false;
      }

      oldEpoch->tod[0] = tod[0];
      oldEpoch->base = ts - range;

      newEpoch->base = ts;
      newEpoch->tod[0] = time(NULL);

      //  Release new Epoch via atomicCAS128

      atomicCAS128(rdtscEpoch, oldEpoch, newEpoch);
    } while (true);
#endif
    // emit assigned Timestamp.

    tsBase[idx].tsEpoch = tod[0];
    tsBase[idx].tsSeqCnt = (uint32_t)units;
  } while (timestampCmp(prev, tsBase + idx) <= 0);
  return;
#endif
#if defined(ATOMIC) || defined(ALIGN)
  tsBase[idx].tsBits[0] = atomicINC64(tsBase->tsBits);
#endif
}

//	install new timestamp if > (or <) existing value

void timestampCAS(Timestamp *dest, Timestamp *src, int chk) {
  Timestamp cmp[1];

  do {
    cmp->tsBits[0] = dest->tsBits[0];
    cmp->tsBits[1] = dest->tsBits[1];

    if (chk > 0 && cmp->tsBits[0] <= src->tsBits[0]) return;

    if (chk < 0 && cmp->tsBits[0] >= src->tsBits[0]) return;

#ifdef _WIN32
  } while (!_InterlockedCompareExchange128 (dest->tsBits, src->tsBits[1], src->tsBits[0], cmp->tsBits));
#else
  } while (!__atomic_compare_exchange(dest->tsBits128, cmp->tsBits128, src->tsBits128, false,
                                      __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE));
#endif
}

  //	install a timestamp value

void installTs(Timestamp *dest, Timestamp *src) {
#ifdef _WIN32
  dest->tsBits[1] = src->tsBits[1];
  dest->tsBits[0] = src->tsBits[0];
#else
  dest->tsBits128[0] = src->tsBits128[0];
#endif
}

int64_t timestampCmp(Timestamp *ts1, Timestamp *ts2) {
#ifdef _WIN32
  int64_t comp;

  if ((comp = ts2->tsBits[1] - ts1->tsBits[1])) return comp;
  return ts2->tsBits[0] - ts1->tsBits[0];
#else
  return ts2->tsBits128[0] - ts1->tsBits128[0];
#endif
}