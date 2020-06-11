/*
 * mainui_setting.c
 *
 *  Created on: 2018年1月11日
 *      Author: tom
 */

#include "mainui_setting.h"
#include "gresource.h"
typedef struct {
	GtkAdjustment *thread_num;
	GtkFileChooserButton *cookie_file;
	GtkListStore *request_head;
	GtkTreeView *tree_view;
} MyMainuiSettingPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(MyMainuiSetting, my_mainui_setting, GTK_TYPE_DIALOG);

void tree_view_name_edit(GtkCellRendererText *renderer, gchar *path,
		gchar *new_text, MyMainuiSetting *self) {
	GtkTreeIter iter;
	GtkTreePath *tpath = gtk_tree_path_new_from_string(path);
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	GtkListStore *list = gtk_tree_view_get_model(priv->tree_view);
	gchar *old_text,*old_value;
	gtk_tree_model_get_iter(list, &iter, tpath);
	gtk_tree_path_free(tpath);
	gtk_tree_model_get(list, &iter, 0, &old_text, 1,&old_value,-1);
	if (new_text != NULL)
		if (g_strcmp0("", new_text) != 0) {
			gtk_list_store_set(list, &iter, 0, new_text, -1);
			if(g_strcmp0(old_value,"... add new value")==0)//clear new row value
				gtk_list_store_set(list,&iter,1,"",-1);

			if (g_strcmp0(old_text, "") == 0) {//appear new row for add new value
				gtk_list_store_append(list, &iter);
				gtk_list_store_set(list, &iter, 0, "", 1, "... add new value", -1);
			}
		} else {
			if (g_strcmp0(old_text, "") != 0)
				gtk_list_store_remove(list, &iter);
		}
	g_free(old_text);
	g_free(old_value);
}
;

void tree_view_value_edit(GtkCellRendererText *renderer, gchar *path,
		gchar *new_text, MyMainuiSetting *self) {
	GtkTreeIter iter;
	GtkTreePath *tpath = gtk_tree_path_new_from_string(path);
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	GtkListStore *list = gtk_tree_view_get_model(priv->tree_view);
	gtk_tree_model_get_iter(list, &iter, tpath);
	gtk_tree_path_free(tpath);
	if (new_text != NULL) {
		gtk_list_store_set(list, &iter, 1, new_text, -1);
	}

}
;

static void my_mainui_setting_class_init(MyMainuiSettingClass *klass) {
	/*
	 * gchar *template;
	 gsize size;
	 g_file_get_contents("mainui_setting.glade",&template,&size,NULL);
	 gtk_widget_class_set_template(klass,g_bytes_new_static(template,size));
	 */
	gtk_widget_class_set_template_from_resource(klass,
			"/org/gtk/mainui_setting.glade");
	gtk_widget_class_bind_template_child_private(klass, MyMainuiSetting,
			thread_num);
	gtk_widget_class_bind_template_child_private(klass, MyMainuiSetting,
			cookie_file);
	gtk_widget_class_bind_template_child_private(klass, MyMainuiSetting,
			tree_view);
	gtk_widget_class_bind_template_child_private(klass, MyMainuiSetting,
			request_head);
	gtk_widget_class_bind_template_callback(klass, tree_view_value_edit);
	gtk_widget_class_bind_template_callback(klass, tree_view_name_edit);
}
;

static void my_mainui_setting_init(MyMainuiSetting *self) {
	gtk_widget_init_template(self);
}

MyMainuiSetting* my_mainui_setting_new(gchar *filename, gint num,
		GtkListStore *list) {
	MyMainuiSetting *set = g_object_new(MY_TYPE_MAINUI_SETTING, NULL);
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(set);
	if (filename != NULL)
		my_mainui_setting_set_cookie_filename(set, filename);
	my_mainui_setting_set_thread_num(set, num);
	if (list != NULL)
		gtk_tree_view_set_model(priv->tree_view, list);
	return set;
}
;
gchar* my_mainui_setting_get_cookie_filename(MyMainuiSetting *self) {
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	return gtk_file_chooser_get_filename(priv->cookie_file);
}
;
gint my_mainui_setting_get_thread_num(MyMainuiSetting *self) {
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	guint num = gtk_adjustment_get_value(priv->thread_num);
	return num;
}
;
gboolean my_mainui_setting_set_cookie_filename(MyMainuiSetting *self,
		gchar *filename) {
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	return gtk_file_chooser_set_filename(priv->cookie_file, filename);
}
;
void my_mainui_setting_set_thread_num(MyMainuiSetting *self, gint num) {
	MyMainuiSettingPrivate *priv = my_mainui_setting_get_instance_private(self);
	gtk_adjustment_set_value(priv->thread_num, (gdouble) num);
}
;
