// ecosense.c - EcoSense live dashboard (Feature 5)
// Usage: ecosense [temp_threshold air_threshold power_threshold]

#include "kernel/types.h"
#include "kernel/sensor.h"
#include "user/user.h"

static void
clear_screen(void)
{
  printf("\x1b[2J\x1b[H");
}

// Print string s left-padded to at least `width` chars.
static void
print_padded(const char *s, int width)
{
  int len = 0;
  const char *p = s;
  while(*p++) len++;
  printf("%s", s);
  while(len++ < width) printf(" ");
}

static const char *
sensor_name(int type)
{
  switch(type){
  case SENSOR_TEMP:  return "Temperature";
  case SENSOR_AIRQ:  return "Air Quality";
  case SENSOR_POWER: return "Energy Usage";
  default:           return "Unknown";
  }
}

static const char *
sensor_unit(int type)
{
  switch(type){
  case SENSOR_TEMP:  return "x0.1C";
  case SENSOR_AIRQ:  return "AQI";
  case SENSOR_POWER: return "W";
  default:           return "?";
  }
}

static int
bar_max(int type)
{
  switch(type){
  case SENSOR_TEMP:  return 400;
  case SENSOR_AIRQ:  return 500;
  case SENSOR_POWER: return 600;
  default:           return 100;
  }
}

static void
print_bar(int value, int max_val, int width)
{
  int filled, i;
  if(max_val <= 0) max_val = 1;
  filled = value * width / max_val;
  if(filled > width) filled = width;
  printf("[");
  for(i = 0; i < width; i++)
    printf("%s", i < filled ? "#" : " ");
  printf("]");
}

static void
print_sensor(struct sensordata *s)
{
  if(s->alert)
    printf("\x1b[31m");

  print_padded(sensor_name(s->type), 13);
  printf("val=%d ", s->value);
  print_padded(sensor_unit(s->type), 6);
  printf("avg=%d  min=%d  max=%d  thr=%d  ",
         s->avg, s->min, s->max, s->threshold);
  print_bar(s->value, bar_max(s->type), 20);

  if(s->alert)
    printf("  \x1b[1m** ALERT **\x1b[0m");

  printf("\n");

  if(s->alert)
    printf("\x1b[0m");
}

int
main(int argc, char *argv[])
{
  struct sensordata buf[NUM_SENSORS];
  int n, i;

  if(argc >= 4){
    setsensorthreshold(SENSOR_TEMP,  atoi(argv[1]));
    setsensorthreshold(SENSOR_AIRQ,  atoi(argv[2]));
    setsensorthreshold(SENSOR_POWER, atoi(argv[3]));
  }

  for(;;){
    clear_screen();
    printf("=== EcoSense Environmental Monitor ===\n\n");
    printf("Sensor         Value  Unit    Avg    Min    Max    Thr    Level\n");
    printf("--------------------------------------------------------------------\n");

    n = readsensors(buf, NUM_SENSORS);
    if(n < 0){
      printf("readsensors() failed.\n");
    } else {
      for(i = 0; i < n; i++)
        print_sensor(&buf[i]);
    }

    printf("\n[Ctrl-A then X to quit QEMU]\n");
    pause(10);
  }

  exit(0);
}
