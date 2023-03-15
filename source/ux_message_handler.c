#include "ux_message_handler.h"


static void ux_manager_send_msg(ux_msg_handler *handler, bool from_isr, ux_msg *input_msg)
{
    if (input_msg == NULL) {
        return ;
    }
    UX_LOG_D("send msg id=%x-%x", input_msg->msg_group, input_msg->msg_id);

    if (handler == NULL || handler->msg_queue == NULL) {
        UX_LOG_E("handler or msg_queue is NULL");
        return;
    }

    handler->msg_queue->push_msg(handler->msg_queue, from_isr, input_msg);
    if (handler->semaphore) {
        ux_ports_semaphore_give(handler->semaphore, from_isr);
    }
}

static void ux_mananger_remove_msg(ux_msg_handler *handler, uint32_t event_group, uint32_t event_id)
{
    if (handler) {
        if (handler->msg_queue) {
            handler->msg_queue->remove_msg(handler->msg_queue, event_group, event_id);
        }

        // if (handler->delay_msg_queue) {
        //     handler->delay_msg_queue->parent.remove_msg(&handler->delay_msg_queue->parent, event_group, event_id);
        // }
    }
}

static void ux_message_task_handler(void *argc)
{
    ux_msg_handler *handler = (ux_msg_handler *)argc;
    ux_msg_queue *queue;
    ux_msg *msg;

    if (handler == NULL) {
        UX_LOG_E("ux message task argc error");
        return;
    }

    queue = handler->msg_queue;
    if (queue == NULL) {
        UX_LOG_E("ux message task queue is NULL");
        return;
    }
    UX_LOG_D("ux message task start");

    while (true) {
        msg = queue->remove_top_msg(queue);
        if (msg) {
            UX_LOG_D("ux message task get top msg %X-%x", msg->msg_group, msg->msg_id);
            ux_ports_free(msg);
        } else {
            UX_LOG_D("ux message task get top msg is NULL");
            ux_ports_semaphore_take(handler->semaphore, UX_PORT_MAX_DELAY);
        }
    }
}


static void ux_message_handler_init(ux_msg_handler *handler)
{
    handler->send_msg = ux_manager_send_msg;
    handler->remove_msg = ux_mananger_remove_msg;

    handler->msg_queue = ux_message_queue_new();

    handler->semaphore = ux_ports_create_semaphore();
    ux_ports_create_task(ux_message_task_handler, "ux_task", 1024 * 4, (void*)handler, UX_PORTS_TASK_PRIO);
}

ux_msg_handler *ux_message_handler_new(void)
{
    ux_msg_handler *handler = ux_ports_malloc(sizeof(ux_msg_handler));
    if (handler) {
        memset(handler, 0x00, sizeof(ux_msg_handler));
        ux_message_handler_init(handler);
    } else {
        UX_LOG_E("message handler new failed, malloc msg is null");
    }
    return handler;
}