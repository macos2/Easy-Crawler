/*
 * task_setting.h
 *
 *  Created on: 2017年12月25日
 *      Author: tom
 */

#ifndef DEBUG_TASK_SETTING_H_
#define DEBUG_TASK_SETTING_H_
#include <glib-object.h>
#include <gtk/gtk.h>
#include "MY_DECLARE.h"
G_BEGIN_DECLS
enum{
	TASK_SOURCE_URL,
	TASK_SOURCE_FILE,
	TASK_SOURCE_LINKER
}TASK_SOURCE;

typedef struct {
	guint source;
	gchar *xpath,*source_name,*fmt_filename,*regex_pattern,*regex_test_text,*fmt_output;
	gboolean search_xpath,output_modify,output_file,terminal_print,title_insist;
}task_set;



#define MY_TYPE_TASK_SETTING my_task_setting_get_type()
MY_DECLARE_DERIVABLE_TYPE(MyTaskSetting,my_task_setting,MY,TASK_SETTING,GtkDialog);
typedef struct _MyTaskSettingClass{
	GtkDialogClass parent_class;
	GArray *(*test)(MyTaskSetting *setting,gchar *regex_pattren,gchar *output_fmt,gchar *eval_content,void *user_data);
};
typedef struct _MyTaskSetting{
	GtkDialog parent_instance;
	gpointer priv;
	task_set set;
};

MyTaskSetting *my_task_setting_new(task_set *set);
void my_task_setting_get_set(MyTaskSetting *self ,task_set *set);

G_END_DECLS


#endif /* DEBUG_TASK_SETTING_H_ */
