#ifndef __BATCH_H
#define __BATCH_H
#include <stdint.h>
#define MAX_APP_NUM 15

typedef struct {
    uint64_t num_apps;
    uint64_t current_app;
    void (*app_start[MAX_APP_NUM + 1])(void);
} AppManager;

extern AppManager *app_manager;
#endif