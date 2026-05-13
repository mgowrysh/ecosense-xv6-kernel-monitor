## EcoSense: Environmental Monitoring on xv6

**Course**: SE3313 – Operating Systems  
**Project type**: Mini project – OS design for environmental sustainability  

---

### 1. Project overview

**EcoSense** is a real-time environmental monitoring subsystem built on top of **xv6-riscv**.  
We simulate environmental sensors as kernel-managed data sources and expose their readings through new system calls. The design emphasizes:

- **Environmental sustainability**: showing how OS mechanisms can support green/IoT monitoring workloads.
- **OS concepts**: synchronization, system calls, kernel data structures, and user-level interfaces.

---

### 2. Sustainability angle

EcoSense targets **design for environmental sustainability**:

- **Simulated sensors** represent temperature, air quality, and energy-usage probes that might exist in a real building or data center.
- **Efficient aggregation** reduces redundant sampling and avoids busy-waiting, illustrating how good OS design can lower CPU utilization and therefore energy use.
- **Centralized policy** in the kernel lets us enforce sampling intervals and alert thresholds, which is how real systems reduce unnecessary work and respond quickly to environmental risks.

---

### 3. Implemented xv6 features (≥4 new features)

All four features are fully implemented and tested inside `xv6-riscv/`.

#### Feature 1 – Kernel sensor syscall interface
- New syscalls: `readsensors(buf, max)` and `setsensorthreshold(id, threshold)`
- `readsensors` copies the latest sensor snapshot (value, avg, min, max, alert flag) into user space
- Syscall numbers 26–27 added to `kernel/syscall.h`, wired through `kernel/syscall.c` and `kernel/sysproc.c`

#### Feature 2 – Kernel-backed sensor simulator
- `kernel/sensor.c` maintains per-sensor ring buffers (`RING_SIZE = 8`)
- A pseudo-random LCG generator produces new values for each sensor every 10 timer ticks (~1 second)
- The simulator is driven by `sensor_tick()` called from `clockintr()` in `kernel/trap.c` — no separate process needed
- Sensors: Temperature (150–350, units of 0.1°C), Air Quality (10–310 AQI), Energy Usage (100–500 W)

#### Feature 3 – Counting semaphores (new synchronization primitive)
- `kernel/sem.c` implements a global semaphore table (`MAX_SEMS = 32`)
- API: `sem_create(init)`, `sem_wait(id)`, `sem_signal(id)`, `sem_destroy(id)`
- Uses xv6's `sleep`/`wakeup` to block waiters without busy-waiting
- Syscall numbers 22–25; fully accessible from user space
- Protects shared sensor ring buffers between the timer-tick writer and `readsensors` readers

#### Feature 4 – Aggregation and threshold alerts
- Each sensor tracks a rolling sum, min, and max across the last `RING_SIZE` samples
- Rolling average = sum / count, computed at read time
- `setsensorthreshold(id, threshold)` configures per-sensor alert thresholds at runtime
- The `alert` field in `struct sensordata` is set to 1 whenever `value > threshold`
- Default thresholds: Temperature 300 (30.0°C), Air Quality 200 AQI, Energy 350 W

#### Feature 5 – User-space live dashboard (`ecosense`)
- `user/ecosense.c` — live text dashboard, refreshes every ~1 second
- Displays value, unit, rolling average, min, max, threshold, and a proportional bar graph per sensor
- Alerts highlighted in red (ANSI) with `** ALERT **` marker
- Optional command-line threshold override: `ecosense <temp> <airq> <power>`

---

### 4. New files added

| File | Purpose |
|---|---|
| `kernel/sensor.h` | Shared `struct sensordata` and sensor type constants |
| `kernel/sensor.c` | Sensor manager, ring buffers, LCG simulator, aggregator, syscall backends |
| `kernel/sem.c` | Counting semaphore table and implementation |
| `user/ecosense.c` | Live dashboard user program |
| `user/semtest.c` | Semaphore unit tests |
| `user/sensortest.c` | Sensor syscall and aggregation unit tests |

#### Modified files

| File | Change |
|---|---|
| `kernel/syscall.h` | Added syscall numbers 22–27 |
| `kernel/syscall.c` | Added extern declarations and dispatch table entries |
| `kernel/sysproc.c` | Added syscall handler wrappers |
| `kernel/defs.h` | Added kernel-internal function prototypes |
| `kernel/main.c` | Added `seminit()` and `sensorinit()` calls at boot |
| `kernel/trap.c` | Added `sensor_tick()` call inside `clockintr()` |
| `user/user.h` | Added user-space syscall declarations |
| `user/usys.pl` | Added syscall stub entries |
| `Makefile` | Added `sem.o`, `sensor.o` to `OBJS`; added new user programs |

---

### 5. Building and running

#### Requirements
- Docker Desktop (recommended — no toolchain install needed)
- OR: QEMU >= 7.2 and `riscv64-linux-gnu-gcc`

#### Build and run with Docker (recommended)

```bash
cd xv6-riscv
docker run --rm -it \
  -v "$(pwd)":/xv6 \
  -w /xv6 \
  ubuntu:24.04 \
  bash -c "apt-get update -qq && \
           apt-get install -y -qq gcc gcc-riscv64-linux-gnu \
             qemu-system-misc make bc && \
           make TOOLPREFIX=riscv64-linux-gnu- CPUS=1 qemu"
```

To quit QEMU: press **Ctrl-A** then **X**.

#### Build without Docker (if toolchain is installed)

```bash
cd xv6-riscv
make TOOLPREFIX=riscv64-linux-gnu- CPUS=1 qemu
```

---

### 6. Demo script

Once xv6 boots to the `$` prompt:

```
$ ecosense
```
Shows the live dashboard. Values update every second; alerts appear in red when thresholds are crossed.

```
$ semtest
```
Runs semaphore unit tests (basic signal/wait, producer-consumer, error handling).

```
$ sensortest
```
Runs sensor unit tests (read, values change over time, alert on/off, aggregation correctness).

---

### 7. Test results

```
$ semtest
=== semtest ===
PASS basic signal/wait
PASS producer-consumer (5 items)
PASS invalid-id error handling
=== all semaphore tests passed ===

$ sensortest
=== sensortest ===
PASS readsensors returned 3 records
PASS 3 sensor(s) changed value after sleep
PASS alert fires when threshold=0
PASS alert clears when threshold=max
PASS rolling average in [min,max] for all sensors
=== all sensor tests passed ===
```


