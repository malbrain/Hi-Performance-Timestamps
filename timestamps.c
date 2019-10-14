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

int tsClientMax; 
bool tsGo = true;

//	atomic install 16 bit value

static bool atomicCAS16(volatile uint16_t *dest, uint16_t comp, uint16_t value) {
#ifdef _WIN32
  return _InterlockedCompareExchange16(dest, value, comp) == comp;
#else
  return __sync_bool_compare_and_swap(dest, comp, value);
#endif
}

//  helper functions

typedef enum {
  tsNop,
  tsEpoch,
  tsScan,
  tsCmd
} tsSrvrCmd; 
  
bool tsCalcEpoch(Timestamp *tsBase) {
time_t tod[1];

  if(time(tod) <= tsBase->tsEpoch)
    return false;
    
  tsBase->tsEpoch = (uint32_t)*tod;
  tsBase->tsSeqCnt = 0;
  return true;
}

bool tsScanReq(Timestamp *tsBase) {
int idx = 0;
  bool result = false;

  while (++idx < tsClientMax)
    if (tsBase[idx].tsCmd == TSGen)
      tsBase[idx].tsBits = ++tsBase->tsBits, result = true;
       
  return result;
}

//  API functions

uint64_t timestampServer(Timestamp *tsBase) {
  uint64_t cycles = 0;
  bool result;
  
  do {
    result = tsCalcEpoch(tsBase);

    if ((result = tsScanReq(tsBase))) continue;

    pause();
    cycles++;
  } while (tsGo);
  return cycles;
}

//	tsMaxClients is the number of client slots plus one for slot zero

void timestampInit(Timestamp *tsArray, int tsMaxClients) {
  tsClientMax = tsMaxClients;
  tsArray->tsBits = 0;
  tsCalcEpoch(tsArray);
}

Timestamp *timestampClnt(Timestamp *tsBase) {
  int idx = 0;

  while (++idx < tsClientMax)
	if( tsBase[idx].tsCmd == TSAvail )
	  if (atomicCAS16(&tsBase[idx].tsCmd, TSAvail, TSIdle)) 
		  return tsBase + idx;

  return NULL;
}

void timestampQuit(Timestamp *timestamp) {
  timestamp->tsCmd = TSAvail;
}

uint64_t timestampNext(Timestamp *timestamp) {

  timestamp->tsCmd = TSGen;

  while (((volatile Timestamp *)(timestamp))->tsCmd == TSGen)
	  pause();

  return timestamp->tsBits;
}

