#ifndef __UX_MANANGER_H__
#define __UX_MANANGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_message_handler.h"

typedef enum {
    UX_STATUS_OUT_OF_RESOURCE = -3,
    UX_STATUS_INVALID_PARAMETER = -2,
    UX_STATUS_INVALID_STATE = -1,
    UX_STATUS_OK = 0,
} ux_status;

typedef struct _ux_manager_handler {
    ux_msg_handler handler;
} ux_manager_handler;

void ux_start(void);

void ux_send_event(bool from_isr, ux_activity_event_priority priority, uint32_t event_group, uint32_t event_id, 
                   void *data, uint32_t data_len, void(*special_free_extra_func)(void), uint32_t delay_ms);

void ux_remove_event(uint32_t event_group, uint32_t event_id);

void ux_set_pre_proc_func(ux_proc_event_func_t proc_func);

void ux_start_activity(ux_activity_context *self, ux_proc_event_func_t proc_func, 
                       ux_activity_priority priority, void *extra_data, uint32_t data_len);
#ifdef __cplusplus
}
#endif

#endif 
