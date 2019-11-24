//	standalone driver file for Timestamps
//	specify /D CLOCK or /D RDTSC on the compile (VS19)
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

extern uint64_t rdtscEpochs;
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
uint64_t idx, dups = 0, prev = 0, count = 0, skipped = 0;;
Timestamp *ts = timestampClnt(tsVector);

printf("Begin client %d\n", args->idx);

for (idx = 0; idx < args->count; idx++) {
  if (timestampNext(ts) > prev)
    count++;
  else if (ts->tsBits == prev)
    dups++;
  else
    skipped++;
  prev = ts->tsBits;
  }

  printf("client %d count = %" PRIu64 " Out of Order = %" PRIu64 " dups = %" PRIu64 "\n", args->idx, count, skipped, dups);
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
  printf("size of Timestamp = %d, TsEpoch = %d\n", (int)sizeof(Timestamp), (int)sizeof(TsEpoch));
  TsArgs *baseArgs, *args;
  uint64_t sum, first = 0, nxt = 0;
  int idx;
  double startx1 = getCpuTime(0);
  double startx2 = getCpuTime(1);
  double startx3 = getCpuTime(2);
  double elapsed;
#ifdef CLOCK
  struct timespec spec[1];
  first = 0;
  first -= __rdtsc();

  for (sum = idx = 0; idx < 1000000; idx++) {
    timespec_get(spec, TIME_UTC);
    sum += spec->tv_nsec;
  }

  first += __rdtsc();
  first /= 1000000;

  elapsed = getCpuTime(0) - startx1;
  startx1 = getCpuTime(0);
  elapsed *= 1000;

  printf("CLOCK UTC timespec cycles per call= %" PRIu64 " %.3f ns\n", first, elapsed);

  first = 0;
  first -= __rdtsc();

  for (sum = idx = 0; idx < 1000000; idx++)
    sum += time(NULL);

  elapsed = getCpuTime(0) - startx1;
  startx1 = getCpuTime(0);
  elapsed *= 1000;

  first += __rdtsc();
  first /= 1000000;

  printf("CLOCK time cycles per call= %" PRIu64 " %.3f ns\n", first, elapsed);
#endif
#ifdef RDTSC
  struct timespec spec[1];
  timespec_get(spec, TIME_UTC);
  uint64_t start = spec->tv_nsec + spec->tv_sec * 1000000000, end;
  nxt = __rdtsc();
  first = __rdtsc() - nxt;

  for (sum = idx = 0; idx < 1000000; idx++) {
    sum -= nxt;
    nxt = __rdtsc();
    sum += nxt;
  }

  timespec_get(spec, TIME_UTC);
  end = spec->tv_nsec + spec->tv_sec * 1000000000;
  printf("RDTSC timing = %" PRIu64 "ns, resolution = %" PRIu64 "\n",
         (end - start)/idx, sum / idx);
  rdtscUnits = sum / idx;
#endif
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
    printf("TSC COUNT: New  Epochs = %" PRIu64 "\n", rdtscEpochs);
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
