/*
 * task_message.h
 *
 *  Created on: 2017年12月30日
 *      Author: tom
 */

#ifndef TASK_MESSAGE_H_
#define TASK_MESSAGE_H_
#include <glib-object.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libsoup/soup.h>
#include "MY_DECLARE.h"

G_BEGIN_DECLS
typedef enum{
	Get_Begin,
	Get_Head,
	Get_Body,
	Get_Finish,
	Get_Cancel,
}Task_Msg_Get_Status;

#define MY_TYPE_TASK_MESSAGE my_task_message_get_type()
MY_DECLARE_DERIVABLE_TYPE(MyTaskMessage,my_task_message,MY,TASK_MESSAGE,GObject);
typedef struct _MyTaskMessageClass{
	GObjectClass parent_class;
};
typedef struct _MyTaskMessage{
	GObject parent_instance;
	SoupURI *uri;
	SoupSession *session;
	xmlDoc *doc;
	xmlXPathContext *ctxt;
	gpointer task;
	gint id;
	gchar *filename, *web_title,*charset,*local,*suggest_filename,*utf8_conv;
	void *list_row;
	gint64 start_time,dl_size,content_size,pre_dlsize;
	gboolean *cancel;
	Task_Msg_Get_Status GET_STATUS;
	SoupStatus soup_status;
	SoupMessage *msg;
	GMutex mutex;
	GString *xpath_result;
};

MyTaskMessage *my_task_message_new(SoupSession *session,SoupURI *uri,gpointer task,gint id);
gboolean my_task_message_free(MyTaskMessage *self);
G_END_DECLS
#endif /* TASK_MESSAGE_H_ */
