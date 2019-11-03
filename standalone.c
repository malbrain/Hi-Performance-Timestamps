//	standalone driver file for Timestamps
//	specify /D QUEUE or /D SCAN on the compile (VS19)
#include "timestamps.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <process.h>
#include <windows.h>
#endif

double getCpuTime(int type);
extern bool tsGo;

typedef struct {
  int idx;
  int count;
} TsArgs;

Timestamp *tsVector;
int maxTS = 8;

#ifndef _WIN32
void *clientGo(void *arg) {
#else
unsigned __stdcall clientGo(void *arg) {
#endif
TsArgs *args = arg;
uint64_t idx, skipped = 0, count = 0;
Timestamp *ts = timestampClnt(tsVector);

printf("Begin client %d\n", args->idx);

for (idx = 0; idx < args->count; idx++) {
	timestampNext(ts);
#ifdef _DEBUG
		if (ts->tsCmd > TSGen)
      count++;
    else
      skipped++;
#endif
  }
#ifdef _DEBUG
  printf("client %d count = %llu skipped = %llu\n", args->idx, count, skipped);
#endif
#ifndef _WIN32
  return NULL;
#else
  return 0;
#endif
}

#ifndef _WIN32
void *serverGo(void *arg) {
#else
unsigned __stdcall serverGo(void *arg) {
#endif
  TsArgs *args = arg;
  uint64_t count;

  count = timestampServer(tsVector);
#ifdef _DEBUG
	printf("server cycles = %llu\n", count);
#endif
#ifndef _WIN32
  return NULL;
#else
  return 0;
#endif
}

#ifndef _WIN32
int main(int argc, char **argv) {
  pthread_t *threads;
  int err;
#else
int _cdecl main(int argc, char **argv) {
  HANDLE *threads;
#endif
  TsArgs *baseArgs, *args;
  int idx;
  double startx1 = getCpuTime(0);
  double startx2 = getCpuTime(1);
  double startx3 = getCpuTime(2);
  double elapsed;

  if (argc > 1) maxTS = atoi(argv[1]) + 1;

  baseArgs = malloc(maxTS * sizeof(TsArgs));

  tsVector = (Timestamp *)calloc(maxTS, sizeof(Timestamp));

  timestampInit(tsVector, maxTS);

#ifndef _WIN32
  threads = malloc(maxTS * sizeof(pthread_t));
#else
  threads = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, maxTS * sizeof(HANDLE));
#endif
  //	fire off threads

  idx = 0;

  do {
    args = baseArgs + idx;
    args->count = atoi(argv[2]);
    args->idx = idx;
#ifndef _WIN32
	if( idx )
      if ((err = pthread_create(threads + idx, NULL, clientGo, args)))
        fprintf(stderr, "Error creating thread %d\n", err);
#if !defined(ATOMIC) && !defined(ALIGN) && !defined(CLOCK) && !defined(RDTSC)
	if( !idx)
      if ((err = pthread_create(threads + idx, NULL, serverGo, args)))
        fprintf(stderr, "Error creating thread %d\n", err);
#endif
#else
	if( idx )
      while (((int64_t)(threads[idx] = (HANDLE)_beginthreadex(NULL, 65536, clientGo, args, 0, NULL)) < 0LL))
        fprintf(stderr, "Error creating thread errno = %d\n", errno);
#if !defined(ATOMIC) && !defined(ALIGN) && !defined(CLOCK) && !defined(RDTSC)
        if (!idx) {
      while (((int64_t)(threads[idx] = (HANDLE)_beginthreadex(NULL, 65536, serverGo, args, 0, NULL)) < 0LL))
          fprintf(stderr, "Error creating thread errno = %d\n", errno);

	  printf("thread %d server for %d timestamps\n", idx, (maxTS - 1) * atoi(argv[2]));
    }
#endif
#endif
	if( idx )
		printf("thread %d launched for %d timestamps\n", idx, atoi(argv[2]));

  } while (++idx < maxTS);

  // 	wait for termination

#ifndef _WIN32
    for (idx = 1; idx < maxTS; idx++) pthread_join(threads[idx], NULL);
#else
    WaitForMultipleObjects(maxTS - 1, threads + 1, TRUE, INFINITE);
	tsGo = false;
#if !!defined(ATOMIC) && !defined(ALIGN)
	WaitForSingleObject(threads[0], INFINITE);
    CloseHandle(threads[0]);
#endif
    for (idx = 1; idx < maxTS; idx++)
		CloseHandle(threads[idx]);
#endif
#ifdef SCAN
    printf("Table Scan\n");
#endif
#ifdef QUEUE
    printf("FIFO Queue\n");
#endif
#ifdef ATOMIC
    printf("Atomic Incr\n");
#endif
#ifdef ALIGN
    printf("Atomic Aligned 64\n");
#endif
#ifdef RDTSC
    printf("TSC COUNT\n");
#endif
#ifdef CLOCK
    printf("Hi Res Timer\n");
#endif
    elapsed = getCpuTime(0) - startx1;
    printf(" real %dm%.3fs\n", (int)(elapsed / 60), elapsed - (int)(elapsed / 60) * 60);

    elapsed = getCpuTime(1) - startx2;
    printf (" user %dm%.3fs\n", (int)(elapsed / 60), elapsed - (int)(elapsed / 60) * 60);

    elapsed = getCpuTime(2) - startx3;
    printf(" sys  %dm%.3fs\n", (int)(elapsed / 60), elapsed - (int)(elapsed / 60) * 60);

    return 0;
}

#ifndef _WIN32
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

double getCpuTime(int type) {
  struct rusage used[1];
  struct timeval tv[1];

  switch (type) {
    case 0:
      gettimeofday(tv, NULL);
      return (double)tv->tv_sec + (double)tv->tv_usec / 1000000;

    case 1:
      getrusage(RUSAGE_SELF, used);
      return (double)used->ru_utime.tv_sec +
             (double)used->ru_utime.tv_usec / 1000000;

    case 2:
      getrusage(RUSAGE_SELF, used);
      return (double)used->ru_stime.tv_sec +
             (double)used->ru_stime.tv_usec / 1000000;
  }

  return 0;
}

#else

#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <windows.h>

double getCpuTime(int type) {
  FILETIME crtime[1];
  FILETIME xittime[1];
  FILETIME systime[1];
  FILETIME usrtime[1];
  SYSTEMTIME timeconv[1];
  double ans = 0;

  memset(timeconv, 0, sizeof(SYSTEMTIME));

  switch (type) {
    case 0:
      GetSystemTimeAsFileTime(xittime);
      FileTimeToSystemTime(xittime, timeconv);
      ans = (double)timeconv->wDayOfWeek * 3600 * 24;
      break;
    case 1:
      GetProcessTimes(GetCurrentProcess(), crtime, xittime, systime, usrtime);
      FileTimeToSystemTime(usrtime, timeconv);
      break;
    case 2:
      GetProcessTimes(GetCurrentProcess(), crtime, xittime, systime, usrtime);
      FileTimeToSystemTime(systime, timeconv);
      break;
  }

  ans += (double)timeconv->wHour * 3600;
  ans += (double)timeconv->wMinute * 60;
  ans += (double)timeconv->wSecond;
  ans += (double)timeconv->wMilliseconds / 1000;
  return ans;
}

#endif
