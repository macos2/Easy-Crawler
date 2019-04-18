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
	self->session = NULL;
	self->filename=NULL;
	self->web_title=NULL;
	self->list_row=NULL;
	self->cancel=NULL;
	self->charset=NULL;
	self->local=NULL;
	self->suggest_filename=NULL;
	self->xpath_result=g_string_new("");
	self->utf8_conv=NULL;
	self->msg=NULL;
	g_mutex_init(&self->mutex);
}
;

MyTaskMessage *my_task_message_new(SoupSession *session,SoupURI *uri,gpointer task,gint id) {
	MyTaskMessage *msg=g_object_new(MY_TYPE_TASK_MESSAGE, NULL);
	msg->session=session;
	msg->uri=uri;
	msg->task=task;
	msg->id=id;
	msg->reply=0;
	return msg;
}
;
gboolean my_task_message_free(MyTaskMessage *self) {
	g_object_unref(self);
	return TRUE;
}
;

void my_task_message_finalize(MyTaskMessage *self) {
	if (self->ctxt != NULL)
		xmlXPathFreeContext(self->ctxt);
	if (self->doc != NULL)
		xmlFreeDoc(self->doc);
	if (self->uri != NULL)
		soup_uri_free(self->uri);
	if(self->filename!=NULL)
		g_free(self->filename);
	if(self->web_title!=NULL)
		g_free(self->web_title);
	if(self->charset!=NULL)g_free(self->charset);
	g_free(self->local);
	g_free(self->suggest_filename);
	g_string_free(self->xpath_result,TRUE);
	g_free(self->utf8_conv);
	if(self->msg!=NULL)g_object_unref(self->msg);
}
;
