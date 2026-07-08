

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 8u

typedef enum {
    TASK_SUSPENDED = 0,  
    TASK_BLOCKED,       
    TASK_READY,          
    TASK_RUNNING         
} task_state_t;

typedef void (*task_func_t)(void);

typedef struct {
    task_func_t   func;        
    task_state_t  state;      
    uint32_t      period_ms;   
    uint32_t      delay_ms;    
    const char   *name;        // for debug
} task_control_block_t;

//Initialise the scheduler
void scheduler_init(void);


int scheduler_add_task(task_func_t func, uint32_t period_ms, const char *name);


void scheduler_tick_handler(void);


void scheduler_run(void);


uint32_t scheduler_get_tick(void);

#endif 