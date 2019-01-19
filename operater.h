/*
 * operater.h
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

#ifndef OPERATER_H_
#define OPERATER_H_

#include <glib-object.h>
#include <gtk/gtk.h>
G_BEGIN_DECLS
#define MY_TYPE_OPERATER my_operater_get_type()
G_DECLARE_DERIVABLE_TYPE(MyOperater,my_operater,MY,OPERATER,GtkBox);
typedef struct _MyOperaterClass{
	GtkBoxClass Parent_class;
	void (*add_child)(MyOperater *self,gpointer userdata);
	void (*close)(MyOperater *self,gpointer userdata);
	void (*rename)(MyOperater *self,gpointer userdata);
};

void my_operater_add(MyOperater *self,GtkWidget *child);
void my_operater_set_parent_layout(MyOperater *self,GtkLayout *layout);
void my_operater_set_title(MyOperater *self,gchar *title);
void my_operater_set_transient_for(MyOperater *self,GtkWindow *parent_window);
gchar* my_operater_get_title(MyOperater *self);
void my_operater_log(MyOperater *self,gchar *log);
MyOperater *my_operater_new(gchar *title);
G_END_DECLS



#endif /* OPERATER_H_ */
