#ifndef CONFIG_ML_H
#define CONFIG_ML_H

#include "contiki.h"
#include "stdbool.h"

extern struct etimer e_timer_ml_pred;

extern unsigned int ml_pred_interval;
extern unsigned int ml_min_pred_interval;

extern bool run_ml_model;

#endif
