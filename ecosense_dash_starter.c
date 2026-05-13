#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#include "ecosense_starter.h"

static void
clear_screen(void)
{
  printf(1, "\x1b[2J\x1b[H");
}

static void
print_sensor_row(struct sensordata *s)
{
  char *name;
  switch (s->type) {
  case SENSOR_TEMPERATURE:
    name = "TEMP";
    break;
  case SENSOR_AIR_QUALITY:
    name = "AIRQ";
    break;
  case SENSOR_ENERGY_USAGE:
    name = "POWER";
    break;
  default:
    name = "UNK ";
    break;
  }

  if (s->alert) {
    printf(1, "\x1b[31m"); // red
  }

  printf(1, "Sensor %-5s  id=%d  value=%d  thresh=%d  alert=%d\n",
         name, s->id, s->value, s->threshold, s->alert);

  if (s->alert) {
    printf(1, "\x1b[0m")
  }
}

int
main(int argc, char *argv[])
{
  struct sensordata buf[16];
  int n;

  for (;;) {
    clear_screen();
    printf(1, "EcoSense Live Dashboard\n");
    printf(1, "=======================\n\n");

    n = readsensors(buf, 16);
    if (n < 0) {
      printf(1, "readsensors() not implemented yet or returned error.\n");
    } else if (n == 0) {
      printf(1, "No sensor data available.\n");
    } else {
      int i;
      for (i = 0; i < n; i++) {
        print_sensor_row(&buf[i]);
      }
    }


    sleep(50); 
  }

  exit();
}

