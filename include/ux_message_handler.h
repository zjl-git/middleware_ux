#ifndef __UX_MESSAGE_HANDLER_H__
#define __UX_MESSAGE_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_ports.h"
#include "ux_message_queue.h"

enum {
    UX_INTERNAL_EVENT_SYSTEM_START,
    UX_INTERNAL_EVENT_SYSTEM_STOP,
    UX_INTERNAL_EVENT_START_ACTI,
    UX_INTERNAL_EVENT_FINISH_ACTI,
    UX_INTERNAL_EVENT_SET_RESULT,
    UX_INTERNAL_EVENT_TRIGGER_REFRESH,
    UX_INTERNAL_EVENT_BACK_TO_IDLE,
    UX_INTERNAL_EVENT_GET_ALLOWN,
    UX_INTERNAL_EVENT_ALLOWED,
};

typedef struct _ux_msg_handler {
    void (*send_msg)(struct _ux_msg_handler *handler, bool from_isr, ux_msg *input_msg);
    // void (*send_delay_msg)(struct _ux_msg_handler *handler, bool from_isr, ux_delay_msg *input_msg);
    void (*handle_msg)(struct _ux_msg_handler *handler, ux_msg *msg);
    void (*remove_msg)(struct _ux_msg_handler *handler, uint32_t event_group, uint32_t event_id);

    ux_msg_queue *msg_queue;
    // ux_delay_msg_queue *delay_msg_queue;

    void *semaphore;
} ux_msg_handler;

ux_msg_handler *ux_message_handler_new(void);

#ifdef __cplusplus
}
#endif

#endif 
