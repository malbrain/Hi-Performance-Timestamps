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
#include <time.h>

//  store the initialization values

Timestamp *tsArray;
int tsClientMax;
bool tsGo = true;

//	atomic install 64 bit value

static bool atomicCAS64(volatile uint64_t *dest, uint64_t comp, uint64_t value) {
#ifdef _WIN32
  return _InterlockedCompareExchange64(dest, value, comp) == comp;
#else
  return __sync_bool_compare_and_swap(dest, comp, value);
#endif
}

//	atomic 64 bit increment

static uint64_t atomicINC64(volatile uint64_t *dest) {
#ifdef _WIN32
  return _InterlockedIncrement64(dest);
#else
  return __sync_add_and_fetch(dest, 1);
#endif
}

//  routine to wait

bool pause(int loops) {
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
  tsBase->tsSeqCnt = 0;
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
      if (!pause(++loops))
		  return false;

    while (tsQueue[tsNext % TSQUEUE] == NULL)
      if (!pause(++loops))
		  return false;

	tsQueue[tsNext % TSQUEUE]->tsBits = ++tsBase->tsBits;
    tsQueue[tsNext % TSQUEUE] = NULL;
    tsTail = tsNext;

    result = true;
  } while (++loops < 1000 || loops < 1000000 && tsHead != tsTail );
  
  return result;
#else
  int idx = 0;

  while (++idx < tsClientMax)
    if (tsBase[idx].tsCmd == TSGen)
      tsBase[idx].tsBits = ++tsBase->tsBits, result = true;
       
  return result;
#endif
}

//  API functions

bool timestampServer(Timestamp *tsBase) {
  int loops = 0;
  bool result;
  
  do {
    result = tsCalcEpoch(tsBase);

    if ((result = tsScanReq(tsBase))) continue;

    if (!pause(++loops)) return false;
  } while (tsGo);
  return true;
}

//	tsMaxClients is the number of client slots plus one for slot zero

void timestampInit(Timestamp *tsBase, int tsMaxClients) {
  tsClientMax = tsMaxClients;
  tsBase->tsBits = 0;
  tsCalcEpoch(tsBase);
  tsArray = tsBase;
#ifdef ALIGN
  tsCache = (Timestamp *)aligned_malloc(64 * tsClientMax, 64);
#endif
}

//  Client request for tsBase slot

Timestamp *timestampClnt(Timestamp *tsBase) {
  int idx = 0;

  while (++idx < tsClientMax)
	if( tsBase[idx].tsCmd == TSAvail )
	  if (atomicCAS64(&tsBase[idx].tsCmd, TSAvail, TSIdle)) 
		  return tsBase + idx;

  return NULL;
}

//	release tsBase slot

void timestampQuit(Timestamp *timestamp) {
  timestamp->tsCmd = TSAvail;
}

//  request next timestamp

uint64_t timestampNext(Timestamp *timestamp) {
#ifdef ATOMIC
	return atomicINC64(&tsArray->tsBits);
#endif
#ifdef ALIGN
    return atomicINC64(&tsCache[8 * (timestamp - tsArray)].tsBits);
#endif
#ifdef QUEUE
  uint64_t tsNext;

  timestamp->tsCmd = TSGen;
  tsNext = atomicINC64(&tsHead);
  tsQueue[tsNext % TSQUEUE] = timestamp;
#endif
#ifdef SCAN
  timestamp->tsCmd = TSGen;
#endif
  int loops = 0;

  while (timestamp->tsCmd == TSGen)
    if (!pause(++loops)) return 0;

  return timestamp->tsBits;
}
