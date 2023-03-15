#include "ux_manager.h"
#include "ux_message_queue.h"

ux_manager_handler *g_manager_handler;

void ux_start(void)
{
    ux_msg msg;
    if (g_manager_handler != NULL) {
        UX_LOG_E("already started a message handler");
        return;
    }

    g_manager_handler = (ux_manager_handler *)ux_message_handler_new();
    if (g_manager_handler == NULL) {
        UX_LOG_E("manager handler new fail");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                               UX_ACTIVITY_SYSTEM_EVENT_ID_CREATE, NULL, 0, NULL, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}

void ux_send_event(bool from_isr, ux_activity_event_priority priority, uint32_t event_group, uint32_t event_id, 
                   void *data, uint32_t data_len, void(*special_free_extra_func)(void), uint32_t delay_ms)
{
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call send event before ux running");
        return;
    }

    if (priority > UX_ACTIVITY_EVENT_PRIORITY_SYSTEM) {
        UX_LOG_E("priority error");
        return ;
    }

    UX_LOG_D("send event: 0x%x, 0x%x, delay:%d", event_group, event_id, delay_ms);
    if (delay_ms != 0) {

    } else {
        ux_msg msg;
        ux_message_queue_input_msg(&msg, priority, event_group, event_id, data, data_len, NULL, 0, NULL);
        g_manager_handler->handler.send_msg(&(g_manager_handler->handler), from_isr, &msg);
    }
}

void ux_remove_event(uint32_t event_group, uint32_t event_id)
{
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call send event before ux running");
        return;
    }

    UX_LOG_I("remove event: 0x%x, 0x%x",event_group, event_id);
    g_manager_handler->handler.remove_msg(&(g_manager_handler->handler), event_group, event_id);
}




void ux_test(void)
{
    ux_msg *msg = g_manager_handler->handler.msg_queue->remove_top_msg(g_manager_handler->handler.msg_queue);
    while (msg) {
        msg = g_manager_handler->handler.msg_queue->remove_top_msg(g_manager_handler->handler.msg_queue);
    }
}