#include "config-ml.h"

unsigned int ml_pred_interval = 20;         // seconds
unsigned int ml_min_pred_interval = 20;     // seconds

bool run_ml_model = true;

struct etimer e_timer_ml_pred;