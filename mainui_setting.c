/*
 * mainui_setting.c
 *
 *  Created on: 2018年1月11日
 *      Author: tom
 */

#include "mainui_setting.h"
#include "gresource.h"
typedef struct{
	GtkAdjustment *thread_num;
	GtkFileChooserButton *cookie_file;
}MyMainuiSettingPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(MyMainuiSetting,my_mainui_setting,GTK_TYPE_DIALOG);
static void my_mainui_setting_class_init(MyMainuiSettingClass *klass){
	/*
	 * gchar *template;
	gsize size;
	g_file_get_contents("mainui_setting.glade",&template,&size,NULL);
	gtk_widget_class_set_template(klass,g_bytes_new_static(template,size));
	*/
gtk_widget_class_set_template_from_resource(klass,"/org/gtk/mainui_setting.glade");
gtk_widget_class_bind_template_child_private(klass,MyMainuiSetting,thread_num);
gtk_widget_class_bind_template_child_private(klass,MyMainuiSetting,cookie_file);
};

static void my_mainui_setting_init(MyMainuiSetting *self){
gtk_widget_init_template(self);
}

MyMainuiSetting *my_mainui_setting_new(gchar *filename,gint num){
MyMainuiSetting *set=g_object_new(MY_TYPE_MAINUI_SETTING,NULL);
if(filename!=NULL)my_mainui_setting_set_cookie_filename(set, filename);
my_mainui_setting_set_thread_num(set,num);
return set;
};
gchar *my_mainui_setting_get_cookie_filename(MyMainuiSetting *self){
MyMainuiSettingPrivate *priv=my_mainui_setting_get_instance_private(self);
return gtk_file_chooser_get_filename(priv->cookie_file);
};
gint my_mainui_setting_get_thread_num(MyMainuiSetting *self){
	MyMainuiSettingPrivate *priv=my_mainui_setting_get_instance_private(self);
	guint num=gtk_adjustment_get_value(priv->thread_num);
	return num;
};
gboolean my_mainui_setting_set_cookie_filename(MyMainuiSetting *self,gchar *filename){
	MyMainuiSettingPrivate *priv=my_mainui_setting_get_instance_private(self);
	return gtk_file_chooser_set_filename(priv->cookie_file,filename);
};
void my_mainui_setting_set_thread_num(MyMainuiSetting *self,gint num){
	MyMainuiSettingPrivate *priv=my_mainui_setting_get_instance_private(self);
	gtk_adjustment_set_value(priv->thread_num,(gdouble)num);
};
