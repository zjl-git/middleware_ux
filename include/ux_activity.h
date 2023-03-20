#ifndef __UX_ACTIVITY_H__
#define __UX_ACTIVITY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_ports.h"

/**
 * |        Terms         |           Details                |
 * |----------------------|----------------------------------|
 * |\b Activity           | A structure that must be implemented by the AP layer to process events. |
 * |\b Pre-proc Activity  | A type of activity that is unique in a project. It pre-processes all events. |
 * |\b Idle Activity      | A type of activity that is normally created when the system starts. |
 * |\b Transient Activity | A type of activity that is created when running. |
 */

typedef enum {
    UX_ACTIVITY_PRE_PROC = 0,
    UX_ACTIVITY_IDLE,
    UX_ACTIVITY_TRANSIENT,
} ux_activity_type;

typedef enum {
    UX_ACTIVITY_PRIORITY_IDLE_BACKGROUND = 0,       /*idle backgroud, only idle activities use it*/
    UX_ACTIVITY_PRIORITY_IDLE_TOP,                  /*idle top, only one idle activity in complete project uses it*/
    UX_ACTIVITY_PRIORITY_LOWSET,                    /*lowset, only transient activities use it*/
    UX_ACTIVITY_PRIORITY_LOW,                       /*low, only transient activities use it*/
    UX_ACTIVITY_PRIORITY_MIDDLE,                    /*middle, only transient activities use it*/
    UX_ACTIVITY_PRIORITY_HIGH,                      /*high, only transient activities use it*/
    UX_ACTIVITY_PRIORITY_HIGHEST,                   /*highest, only transient activities use it*/
} ux_activity_priority;

typedef enum {
    UX_ACTIVITY_SYSTEM = 0,                        /*only ux internal use*/
    UX_ACTIVITY_APP_BASE,                           /**/
} ux_activity_event_group;

typedef enum {
    UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_START,                   /*created and will be added*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_STOP,                    /*destroy and removed form activity stack*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_START_ACTI,                     /*resuming*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_FINISH_ACTI,                    /*pausing*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_SET_RESULT,                     /*receiving a result*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH,                /*refreshing*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_REQUEST_ALLOWANCE,              /*request allowance*/
    UX_ACTIVITY_SYSTEM_EVENT_ID_ON_ALLOWANCE,              /*on allowance*/
} ux_activity_event_id;

typedef enum {
    UX_ACTIVITY_EVENT_PRIORITY_LOWSET = 0,
    UX_ACTIVITY_EVENT_PRIORITY_LOW,
    UX_ACTIVITY_EVENT_PRIORITY_MIDDLE,
    UX_ACTIVITY_EVENT_PRIORITY_HIGH,
    UX_ACTIVITY_EVENT_PRIORITY_HIGHEST,
    UX_ACTIVITY_EVENT_PRIORITY_SYSTEM,
} ux_activity_event_priority;

typedef struct _ux_activity_context {
    void *local_context;
} ux_activity_context;

typedef bool (*ux_proc_event_func_t)(ux_activity_context *self, uint32_t event_group, uint32_t event_id, void *extra_data, uint32_t data_len);

/*This structure defines the activity structure. The activity structure should be malloced by ui shell*/
typedef  struct _ux_activity_internal {
    ux_activity_context external_activity;                  /*The external activity structure*/
    ux_activity_type activity_type;                         /*The activity type*/
    ux_activity_priority priority;                          /*The activity priority*/
    struct _ux_activity_internal *started_by;               /*Record which activity start current activity, can be NULL*/
    struct _ux_activity_internal *next;                     /*The next activity in activities stack*/
    ux_proc_event_func_t proc_event_func;                   /*The process event callback function, must be implemented by AP*/
} ux_activity_internal;

ux_activity_internal *ux_activity_get_preproc_item(void);

ux_activity_internal *ux_activity_get_free_item(void);

void ux_activity_free_item(ux_activity_internal *activity);

bool ux_activity_add(ux_activity_internal *activity, void *data, size_t data_len);

void ux_activity_remove(ux_activity_internal *activity);

void ux_activity_start(void);

void ux_activity_set_result(ux_activity_internal *target_acti, void *data, uint32_t data_len);

void ux_activity_refresh_activity(ux_activity_internal *target_acti);

void ux_activity_traverse(uint32_t event_group, uint32_t event_id, void *data, uint32_t data_len);

#ifdef __cplusplus
}
#endif

#endif 
