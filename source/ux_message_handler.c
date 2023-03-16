#include "ux_message_handler.h"

static void ux_manager_handle_msg(ux_msg_handler *handler, ux_msg *msg)
{
    ux_activity_internal *activity;
    UX_LOG_D("handle message group : 0x%x, id : 0x%x, extra_data : 0x%x, data_len : 0x%x, free_func : 0x%x",
              msg->msg_group, msg->msg_id, msg->msg_data, msg->msg_data_len, msg->special_free_extra_func);

    switch (msg->msg_group) {
        case UX_ACTIVITY_SYSTEM:
            activity = (ux_activity_internal *)msg->msg_data2;
            switch (msg->msg_id) {
                case UX_ACTIVITY_SYSTEM_EVENT_ID_CREATE:
                    ux_activity_start();
                    break;

                case UX_ACTIVITY_SYSTEM_EVENT_ID_RESUME:
                    ux_activity_add(activity, msg->msg_data, msg->msg_data_len);
                    if (msg->msg_data && msg->msg_data_len) {
                        ux_ports_free(msg->msg_data);
                    }
                    break;

                case UX_ACTIVITY_SYSTEM_EVENT_ID_PAUSE:
                    break;

                default:
                    break;
            }
            break;

        default:
            ux_activity_traverse(msg->msg_group, msg->msg_id, msg->msg_data, msg->msg_data_len);
            UX_LOG_I("handle message group end: 0x%x, id : 0x%x, extra_data : 0x%x, data_len : 0x%x, free_func : 0x%x",
                      msg->msg_group, msg->msg_id, msg->msg_data, msg->msg_data_len, msg->special_free_extra_func);

            if (msg->msg_data || msg->msg_data_len) {
                if (msg->special_free_extra_func) {
                    msg->special_free_extra_func(msg->msg_data);
                }
                ux_ports_free(msg->msg_data);
            }
            break;
    }
}

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

static void ux_manager_send_delay_msg(ux_msg_handler *handler, bool from_isr, ux_delay_msg *input_msg)
{
    if (input_msg == NULL) {
        return;
    }
    UX_LOG_D("send msg id=%x-%x, time:%d", input_msg->basic_msg.msg_group, input_msg->basic_msg.msg_id, input_msg->msg_act_time);

    if (handler == NULL || handler->delay_msg_queue == NULL) {
        UX_LOG_E("handler or delay_msg_queue is NULL");
        return;
    }

    handler->delay_msg_queue->push_delay_msg(handler->delay_msg_queue, input_msg);
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

        if (handler->delay_msg_queue) {
            handler->delay_msg_queue->parent.remove_msg(&handler->delay_msg_queue->parent, event_group, event_id);
        }
    }
}

static void ux_message_task_handler(void *argc)
{
    ux_msg_handler *handler = (ux_msg_handler *)argc;
    ux_msg_queue *queue;
    ux_msg *msg, *delay_msg, *last_delay_msg;
    uint32_t next_delay_ms = 0;

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
            handler->handle_msg(handler, msg);
            ux_ports_free(msg);
            continue;
        } 

        next_delay_ms = handler->delay_msg_queue->get_next_timeout(handler->delay_msg_queue);
        if (next_delay_ms == UX_PORT_MAX_DELAY) {
            if (handler->semaphore) {
                ux_ports_semaphore_take(handler->semaphore, UX_PORT_MAX_DELAY);
            }
        } else if (next_delay_ms != 0) {
            if (handler->semaphore) {
                ux_ports_semaphore_take(handler->semaphore, next_delay_ms);
            }
        } else {
            delay_msg = &(handler->delay_msg_queue->remove_timeout_msg(handler->delay_msg_queue)->basic_msg);
            while(delay_msg) {
                handler->handle_msg(handler, delay_msg);
                last_delay_msg = delay_msg;
                delay_msg = delay_msg->next_msg;
                ux_ports_free(last_delay_msg);
            }
        }
    }
}


static void ux_message_handler_init(ux_msg_handler *handler)
{
    handler->send_msg = ux_manager_send_msg;
    handler->send_delay_msg = ux_manager_send_delay_msg;
    handler->remove_msg = ux_mananger_remove_msg;
    handler->handle_msg = ux_manager_handle_msg;

    handler->msg_queue = ux_message_queue_new();
    handler->delay_msg_queue = ux_message_delay_queue_new();

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