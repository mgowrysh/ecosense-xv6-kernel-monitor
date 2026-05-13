#ifndef ECOSENSE_STARTER_H
#define ECOSENSE_STARTER_H

enum sensor_type {
  SENSOR_TEMPERATURE = 0,
  SENSOR_AIR_QUALITY = 1,
  SENSOR_ENERGY_USAGE = 2,
  SENSOR_COUNT
};

struct sensordata {
  int id;           
  int type;        
  int value;       
  int threshold;   
  int alert;        
};

int readsensors(struct sensordata *buf, int max);

int setsensorthreshold(int id, int threshold);

int sem_create(int initial);   
int sem_wait(int semid);       
int sem_signal(int semid);     
int sem_destroy(int semid);    

#endif 

