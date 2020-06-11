/*
 * mainui_setting.h
 *
 *  Created on: 2018年1月11日
 *      Author: tom
 */

#ifndef MAINUI_SETTING_H_
#define MAINUI_SETTING_H_
#include <glib-object.h>
#include <gtk/gtk.h>
G_BEGIN_DECLS
#define MY_TYPE_MAINUI_SETTING my_mainui_setting_get_type()
G_DECLARE_DERIVABLE_TYPE(MyMainuiSetting,my_mainui_setting,MY,MAINUI_SETTING,GtkDialog);
typedef struct _MyMainuiSettingClass{
	GtkDialogClass parent_class;
};
MyMainuiSetting *my_mainui_setting_new(gchar *filename,gint num,GtkListStore *list);
gchar *my_mainui_setting_get_cookie_filename(MyMainuiSetting *self);
gint my_mainui_setting_get_thread_num(MyMainuiSetting *self);
gboolean my_mainui_setting_set_cookie_filename(MyMainuiSetting *self,gchar *filename);
void my_mainui_setting_set_thread_num(MyMainuiSetting *self,gint num);
G_END_DECLS
#endif /* MAINUI_SETTING_H_ */
