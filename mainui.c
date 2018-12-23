/*
 * ui.c
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

#include "mainui.h"
#include "gresource.h"
typedef struct {
GString *name;
GtkLayout *layout;
GtkStatusbar *status_bar;
} MyMainuiPrivate;

G_DEFINE_TYPE_WITH_CODE(MyMainui, my_mainui, GTK_TYPE_WINDOW,
		G_ADD_PRIVATE(MyMainui));

void my_mainui_add_clicked(GtkToolButton *button,gpointer userdata);
void my_mainui_info_clicked(GtkToolButton *button,gpointer userdata);
void my_mainui_exec_clicked(GtkToolButton *button,gpointer userdata);
void my_mainui_stop_clicked(GtkToolButton *button,gpointer userdata);
void mainui_open_clicked (GtkMenuItem *item, MyMainui *MyMainui);
void mainui_save_clicked (GtkMenuItem *item, MyMainui *MyMainui);
void mainui_setting_clicked (GtkMenuItem *item, MyMainui *MyMainui);

static void my_mainui_class_init(MyMainuiClass *klass) {
	/*gchar*template;
	gsize size;
	g_file_get_contents("mainui.glade", &template, &size, NULL);
	gtk_widget_class_set_template(klass, g_bytes_new_static(template, size));*/
	gtk_widget_class_set_template_from_resource(klass,"/org/gtk/mainui.glade");
	gtk_widget_class_bind_template_child_private(klass,MyMainui,layout);
	gtk_widget_class_bind_template_child_private(klass,MyMainui,status_bar);
	gtk_widget_class_bind_template_callback(klass,my_mainui_show_msg);
	gtk_widget_class_bind_template_callback(klass,my_mainui_add_clicked);
	gtk_widget_class_bind_template_callback(klass,my_mainui_info_clicked);
	gtk_widget_class_bind_template_callback(klass,my_mainui_exec_clicked);
	gtk_widget_class_bind_template_callback(klass,mainui_open_clicked);
	gtk_widget_class_bind_template_callback(klass,mainui_save_clicked);
	gtk_widget_class_bind_template_callback(klass,mainui_setting_clicked);
	gtk_widget_class_bind_template_callback(klass,my_mainui_stop_clicked);
	g_signal_new("add_child",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,add_child),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("info",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,info),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("exec",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,exec),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("stop",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,stop),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("open",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,open),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("save",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,save),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("setting",MY_TYPE_MAINUI,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyMainuiClass,setting),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);

}
;

static void my_mainui_init(MyMainui *self) {
	gtk_widget_init_template(self);
MyMainuiPrivate *priv=my_mainui_get_instance_private(self);
priv->name=g_string_new("My UI");
}
;

void my_mainui_show_msg(GtkToolButton *button,gpointer userdata){
g_print("%s Clicked\n",gtk_tool_button_get_label(button));
};
MyMainui *my_mainui_new(){
	MyMainui *ui=g_object_new(MY_TYPE_MAINUI, NULL);
	gtk_widget_show_all(ui);
	return ui;
};
void my_mainui_add_clicked(GtkToolButton *button,gpointer userdata){
g_signal_emit_by_name(userdata,"add_child",NULL);
};
void my_mainui_info_clicked(GtkToolButton *button,gpointer userdata){
	g_signal_emit_by_name(userdata,"info",NULL);
};

void my_mainui_exec_clicked(GtkToolButton *button,gpointer userdata){
	g_signal_emit_by_name(userdata,"exec",NULL);
};

void my_mainui_stop_clicked(GtkToolButton *button,gpointer userdata){
	g_signal_emit_by_name(userdata,"stop",NULL);
};

void mainui_open_clicked (GtkMenuItem *item, MyMainui *MyMainui){
	g_signal_emit_by_name(MyMainui,"open",NULL);
};
void mainui_save_clicked (GtkMenuItem *item, MyMainui *MyMainui){
	g_signal_emit_by_name(MyMainui,"save",NULL);
};

void mainui_setting_clicked (GtkMenuItem *item, MyMainui *MyMainui){
	g_signal_emit_by_name(MyMainui,"setting",NULL);
};



GtkLayout *my_mainui_get_layout(MyMainui *self){
	MyMainuiPrivate *priv=my_mainui_get_instance_private(self);
	return priv->layout;
};

GtkStatusbar *my_mainui_get_statusbar(MyMainui *self){
	MyMainuiPrivate *priv=my_mainui_get_instance_private(self);
	return priv->status_bar;
};
