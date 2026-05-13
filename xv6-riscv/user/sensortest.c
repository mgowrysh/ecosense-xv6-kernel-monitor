// sensortest.c - unit test for sensor syscalls (Features 1, 2, 4)
//
// Tests:
//   1. readsensors() returns NUM_SENSORS records.
//   2. Values change between calls (simulator is running).
//   3. setsensorthreshold() + alert flag fires on extreme threshold.
//   4. Rolling average stays within [min, max].

#include "kernel/types.h"
#include "kernel/sensor.h"
#include "user/user.h"

static void
test_read(void)
{
  struct sensordata buf[NUM_SENSORS];
  int n = readsensors(buf, NUM_SENSORS);
  if(n != NUM_SENSORS){
    printf("FAIL: readsensors returned %d, expected %d\n", n, NUM_SENSORS);
    exit(1);
  }
  printf("PASS readsensors returned %d records\n", n);
}

static void
test_values_change(void)
{
  struct sensordata a[NUM_SENSORS], b[NUM_SENSORS];
  int changed, i;

  readsensors(a, NUM_SENSORS);
  pause(15);
  readsensors(b, NUM_SENSORS);

  changed = 0;
  for(i = 0; i < NUM_SENSORS; i++){
    if(a[i].value != b[i].value || a[i].avg != b[i].avg)
      changed++;
  }

  if(changed == 0){
    printf("FAIL: sensor values did not change after sleep\n");
    exit(1);
  }
  printf("PASS %d sensor(s) changed value after sleep\n", changed);
}

static void
test_threshold_alert(void)
{
  struct sensordata buf[NUM_SENSORS];
  int orig;

  // Save original threshold and set it to 0 (always alert)
  readsensors(buf, NUM_SENSORS);
  orig = buf[SENSOR_TEMP].threshold;

  setsensorthreshold(SENSOR_TEMP, 0);
  pause(5);
  readsensors(buf, NUM_SENSORS);
  if(!buf[SENSOR_TEMP].alert){
    printf("FAIL: alert should be set when threshold=0\n");
    setsensorthreshold(SENSOR_TEMP, orig);
    exit(1);
  }
  printf("PASS alert fires when threshold=0 (value=%d)\n", buf[SENSOR_TEMP].value);

  // Restore to a huge threshold - alert should clear
  setsensorthreshold(SENSOR_TEMP, 0x7fffffff);
  pause(5);
  readsensors(buf, NUM_SENSORS);
  if(buf[SENSOR_TEMP].alert){
    printf("FAIL: alert should be clear when threshold=max\n");
    setsensorthreshold(SENSOR_TEMP, orig);
    exit(1);
  }
  printf("PASS alert clears when threshold=max\n");

  setsensorthreshold(SENSOR_TEMP, orig);
}

static void
test_aggregation(void)
{
  struct sensordata buf[NUM_SENSORS];
  int i;
  readsensors(buf, NUM_SENSORS);
  for(i = 0; i < NUM_SENSORS; i++){
    if(buf[i].avg < buf[i].min || buf[i].avg > buf[i].max){
      printf("FAIL: sensor %d avg=%d not in [%d,%d]\n",
             i, buf[i].avg, buf[i].min, buf[i].max);
      exit(1);
    }
  }
  printf("PASS rolling average in [min,max] for all sensors\n");
}

int
main(void)
{
  printf("\n=== sensortest ===\n");
  test_read();
  test_values_change();
  test_threshold_alert();
  test_aggregation();
  printf("=== all sensor tests passed ===\n\n");
  exit(0);
}
