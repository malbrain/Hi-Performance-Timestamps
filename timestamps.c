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

//  store the initialization values

int tsClientMax; 

//  helper functions

typedef enum {
  tsNop,
  tsEpoch,
  tsScan,
  tsCmd
} tsSrvrCmd; 
  
bool tsCalcEpoch(Timestamp *tsBase) {
time_t tod[1];

  if(time(tod) <= tsBase->epoch)
    return false;
    
  tsBase->epoch = (uint32_t)*tod;
  return true;

}

bool tsScanReq(Timestamp *tsBase) {
int idx = 0;

  while( ++idx < tsClientMax )
    if( tsBase[idx].cmd == TSGen )
      tsBase[idx].tsBits = ++tsBase->tsBits;
      
  return true;
}

void tsServer(Timestamp *tsBase) {
bool result;
int cmd;

  do {
    for( cmd = 0; cmd < tsCmd; cmd++ )
      switch (cmd) {
      case tsEpoch:
        if( (result = tsCalcEpoch(tsBase) )
          continue;
        break;
      case tsScan:
        if( (result = tsScanReq(tsBase) )
          continue;
        break;
      }
      break;
  } while( ts );
}

//  API functions

void timestampInit(Timestamp *tsArray, int tsMaxClients, bool tsServer) {
  tsClientMax = tsMaxClients;
}

Timestamp *timestampClnt(Timestamp *tsArray) {
}

void timestampQuit(Timestamp *timestamp) {
}

uint64_t timestampNext(Timestamp *timestamp) {
}
