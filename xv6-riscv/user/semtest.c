// semtest.c - unit test for counting semaphores (Feature 3)
//
// Tests:
//   1. Basic sem_create / sem_signal / sem_wait round-trip.
//   2. Producer-consumer pair: producer signals N times, consumer waits N times.
//   3. Invalid-id error handling.

#include "kernel/types.h"
#include "user/user.h"

#define N 5

static void
test_basic(void)
{
  int sid = sem_create(0);
  if(sid < 0){ printf("FAIL: sem_create returned %d\n", sid); exit(1); }

  // signal then wait - should not block
  if(sem_signal(sid) < 0){ printf("FAIL: sem_signal\n"); exit(1); }
  if(sem_wait(sid)   < 0){ printf("FAIL: sem_wait\n");   exit(1); }

  sem_destroy(sid);
  printf("PASS basic signal/wait\n");
}

static void
test_producer_consumer(void)
{
  int sid = sem_create(0);
  int pid;

  if(sid < 0){ printf("FAIL: sem_create\n"); exit(1); }

  pid = fork();
  if(pid < 0){ printf("FAIL: fork\n"); exit(1); }

  if(pid == 0){
    // child = producer: signal N times with a small delay between each
    int i;
    for(i = 0; i < N; i++){
      pause(2);
      sem_signal(sid);
    }
    exit(0);
  } else {
    // parent = consumer: wait N times and print progress
    int i;
    for(i = 0; i < N; i++){
      sem_wait(sid);
      printf("  consumer received item %d\n", i+1);
    }
    wait(0);
  }

  sem_destroy(sid);
  printf("PASS producer-consumer (%d items)\n", N);
}

static void
test_invalid(void)
{
  if(sem_wait(-1) == 0){ printf("FAIL: sem_wait(-1) should return -1\n"); exit(1); }
  if(sem_signal(999) == 0){ printf("FAIL: sem_signal(999) should return -1\n"); exit(1); }
  printf("PASS invalid-id error handling\n");
}

int
main(void)
{
  printf("\n=== semtest ===\n");
  test_basic();
  test_producer_consumer();
  test_invalid();
  printf("=== all semaphore tests passed ===\n\n");
  exit(0);
}
