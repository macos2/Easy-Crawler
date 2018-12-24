/*
 * task_message.c
 *
 *  Created on: 2017年12月30日
 *      Author: tom
 */

#include "task_message.h"

G_DEFINE_TYPE(MyTaskMessage, my_task_message, G_TYPE_OBJECT);
void my_task_message_finalize(MyTaskMessage *self);
static void my_task_message_class_init(MyTaskMessageClass *klass) {
	GObjectClass *obj_class = klass;
	obj_class->finalize = my_task_message_finalize;
}
;
static void my_task_message_init(MyTaskMessage *self) {
	self->uri = NULL;
	self->ctxt = NULL;
	self->doc = NULL;
	self->msg = NULL;
	self->session = NULL;
}
;

MyTaskMessage *my_task_message_new(SoupSession *session,SoupURI *uri,gpointer task,gint id) {
	MyTaskMessage *msg=g_object_new(MY_TYPE_TASK_MESSAGE, NULL);
	msg->session=g_object_ref(session);
	msg->uri=uri;
	msg->task=task;
	msg->id=id;
	return msg;
}
;
gboolean my_task_message_free(MyTaskMessage *self) {
	g_object_unref(self);
	return TRUE;
}
;

void my_task_message_finalize(MyTaskMessage *self) {
	g_object_unref(self->session);
	if (self->ctxt != NULL)
		xmlXPathFreeContext(self->ctxt);
	if (self->doc != NULL)
		xmlFreeDoc(self->doc);
	if (self->msg != NULL)
		g_object_unref(self->msg);
	if (self->uri != NULL)
		soup_uri_free(self->uri);
}
;
