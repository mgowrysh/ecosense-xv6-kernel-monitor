// sensor.h - shared sensor data structures for kernel and user space
// Include as "kernel/sensor.h" from user programs.

#ifndef SENSOR_H
#define SENSOR_H

#define NUM_SENSORS   3
#define SENSOR_TEMP   0   // temperature  (degrees * 10, e.g. 250 = 25.0 C)
#define SENSOR_AIRQ   1   // air quality  (AQI, 0-500)
#define SENSOR_POWER  2   // energy usage (watts)

struct sensordata {
  int id;         // sensor index (0..NUM_SENSORS-1)
  int type;       // one of SENSOR_* above
  int value;      // current reading
  int avg;        // rolling average over last RING_SIZE samples
  int min;        // minimum seen since boot
  int max;        // maximum seen since boot
  int threshold;  // alert fires when value > threshold (0 = disabled)
  int alert;      // 1 if value > threshold, else 0
};

#endif // SENSOR_H
