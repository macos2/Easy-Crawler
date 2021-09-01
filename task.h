/*
 * task.h
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

#ifndef TASK_H_
#define TASK_H_

#include <glib-object.h>
#include <gtk/gtk.h>
G_BEGIN_DECLS
#define task_state_file "accessories-text-editor"
#define task_state_url "insert-link"
#define task_state_xpath "system-search"
#define task_state_none ""
#define task_state_print "utilities-terminal"
typedef enum{
	task_name=1,
	task_state,
	task_label,
	task_icon,
	task_next,
	task_n_prop
}MY_TASK_PROP;

#define MY_TYPE_TASK  my_task_get_type()
G_DECLARE_DERIVABLE_TYPE(MyTask,my_task,MY,TASK,GtkBox);
typedef struct _MyTaskClass{
	GtkBoxClass Parent_class;
	void (*remove_task)(MyTask *self,GtkWidget *parent);
	void (*content_clicked)(MyTask *self);
	void (*link_del)(MyTask *self);

};
MyTask *my_task_new();
GtkImage *my_task_get_next_icon(MyTask *self);
GtkImage *my_task_get_state_icon(MyTask *self);
GtkImage *my_task_get_icon(MyTask *self);
GtkLabel *my_task_get_label(MyTask *self);
GtkEventBox *my_task_get_next_event(MyTask *self);
GtkEventBox *my_task_get_state_event(MyTask *self);
G_END_DECLS


#endif /* TASK_H_ */
