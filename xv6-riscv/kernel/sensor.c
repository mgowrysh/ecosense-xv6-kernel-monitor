// sensor.c - EcoSense kernel sensor manager for xv6-riscv
//
// Features implemented here:
//   Feature 1 - readsensors() / setsensorthreshold() kernel interface
//   Feature 2 - kernel-side sensor simulator driven by timer ticks
//   Feature 4 - rolling aggregation (average, min, max) and threshold alerts
//
// Synchronization (Feature 3 - semaphores in sem.c) protects the ring buffers
// against concurrent readers and the timer-tick writer.

#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "sensor.h"

// ── ring buffer ────────────────────────────────────────────────────────────

#define RING_SIZE  8
#define TICK_INTERVAL 10   // update sensors every 10 timer ticks (~1 second)

struct ringbuf {
  int values[RING_SIZE];
  int head;    // index of oldest entry
  int count;   // how many entries are filled (0..RING_SIZE)
};

// ── per-sensor state ────────────────────────────────────────────────────────

struct sensorstate {
  struct ringbuf  buf;
  int             threshold;  // 0 = disabled
  int             min;
  int             max;
  long            sum;        // sum of all buf.count samples for rolling avg
  struct spinlock lock;
};

static struct sensorstate sensors[NUM_SENSORS];
static int tick_counter;
static int sensor_ready = 0;  // set to 1 by sensorinit(); guards sensor_tick()

// ── simple LCG pseudo-random generator (no libc in kernel) ─────────────────

static uint lcg_seed = 98765;

static int
lcg_rand(void)
{
  lcg_seed = lcg_seed * 1664525 + 1013904223;
  return (int)(lcg_seed >> 1) & 0x7fffffff;
}

// ── helpers ─────────────────────────────────────────────────────────────────

static int
sensor_base(int type)
{
  switch(type){
  case SENSOR_TEMP:  return 150;   // 15.0 C
  case SENSOR_AIRQ:  return 10;    // AQI 10
  case SENSOR_POWER: return 100;   // 100 W
  default:           return 0;
  }
}

static int
sensor_range(int type)
{
  switch(type){
  case SENSOR_TEMP:  return 200;   // up to 35.0 C
  case SENSOR_AIRQ:  return 300;   // up to AQI 310
  case SENSOR_POWER: return 400;   // up to 500 W
  default:           return 1;
  }
}

static int
default_threshold(int type)
{
  switch(type){
  case SENSOR_TEMP:  return 300;   // 30.0 C
  case SENSOR_AIRQ:  return 200;   // AQI 200
  case SENSOR_POWER: return 350;   // 350 W
  default:           return 0;
  }
}

// ── push one reading into a sensor's ring buffer ────────────────────────────

static void
ring_push(struct sensorstate *s, int value)
{
  int idx;

  if(s->buf.count < RING_SIZE){
    idx = (s->buf.head + s->buf.count) % RING_SIZE;
    s->buf.count++;
  } else {
    // overwrite oldest
    idx = s->buf.head;
    s->sum -= s->buf.values[idx];
    s->buf.head = (s->buf.head + 1) % RING_SIZE;
  }
  s->buf.values[idx] = value;
  s->sum += value;

  if(s->buf.count == 1 || value < s->min) s->min = value;
  if(s->buf.count == 1 || value > s->max) s->max = value;
}

// ── public init ─────────────────────────────────────────────────────────────

void
sensorinit(void)
{
  int i, v;
  for(i = 0; i < NUM_SENSORS; i++){
    initlock(&sensors[i].lock, "sensor");
    sensors[i].threshold = default_threshold(i);
    sensors[i].min       = 0x7fffffff;
    sensors[i].max       = 0;
    sensors[i].sum       = 0;
    sensors[i].buf.head  = 0;
    sensors[i].buf.count = 0;
    v = sensor_base(i) + lcg_rand() % sensor_range(i);
    ring_push(&sensors[i], v);
  }
  tick_counter = 0;
  sensor_ready = 1;
}

// ── called from clockintr() every timer tick ────────────────────────────────

void
sensor_tick(void)
{
  int i, v;

  if(!sensor_ready)
    return;

  tick_counter++;
  if(tick_counter < TICK_INTERVAL)
    return;
  tick_counter = 0;

  for(i = 0; i < NUM_SENSORS; i++){
    v = sensor_base(i) + lcg_rand() % sensor_range(i);
    acquire(&sensors[i].lock);
    ring_push(&sensors[i], v);
    release(&sensors[i].lock);
  }
}

// ── readsensors: copy latest snapshot into user space ───────────────────────
// Returns number of sensors written, or -1.

int
sensor_read(struct sensordata *ubuf, int max)
{
  int i, n, latest_idx;
  struct sensordata tmp[NUM_SENSORS];

  n = (max < NUM_SENSORS) ? max : NUM_SENSORS;
  for(i = 0; i < n; i++){
    acquire(&sensors[i].lock);

    // grab the most-recently-pushed value
    if(sensors[i].buf.count == 0){
      release(&sensors[i].lock);
      tmp[i].value = 0;
    } else {
      latest_idx = (sensors[i].buf.head + sensors[i].buf.count - 1) % RING_SIZE;
      tmp[i].value     = sensors[i].buf.values[latest_idx];
      tmp[i].avg       = (int)(sensors[i].sum / sensors[i].buf.count);
      tmp[i].min       = sensors[i].min;
      tmp[i].max       = sensors[i].max;
      tmp[i].threshold = sensors[i].threshold;
      tmp[i].alert     = (tmp[i].value > sensors[i].threshold) ? 1 : 0;
      tmp[i].id        = i;
      tmp[i].type      = i;
      release(&sensors[i].lock);
    }
  }

  // copy out to user space
  if(copyout(myproc()->pagetable, (uint64)ubuf,
             (char*)tmp, n * sizeof(struct sensordata)) < 0)
    return -1;

  return n;
}

// ── setsensorthreshold ──────────────────────────────────────────────────────

int
sensor_setthreshold(int id, int threshold)
{
  if(id < 0 || id >= NUM_SENSORS)
    return -1;
  acquire(&sensors[id].lock);
  sensors[id].threshold = threshold;
  release(&sensors[id].lock);
  return 0;
}
