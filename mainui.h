/*
 * ui.h
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

#ifndef MAINUI_H_I_H_
#define MAINUI_H_AINUI_H_
#include <gtk/gtk.h>
#include <glib-object.h>
G_BEGIN_DECLS
#define  MY_TYPE_MAINUI my_mainui_get_type()
G_DECLARE_DERIVABLE_TYPE(MyMainui,my_mainui,MY,MAINUI,GtkWindow);
typedef struct _MyMainuiClass{
	GtkWindowClass Parent_class;
	void (*add_child)(MyMainui *ui,gpointer userdata);
	void (*info)(MyMainui *ui,gpointer userdata);
	void (*exec)(MyMainui *ui,gpointer userdata);
	void (*open)(MyMainui *ui,gpointer userdata);
	void (*save)(MyMainui *ui,gpointer userdata);
	void (*setting)(MyMainui *ui,gpointer userdata);
	void (*stop)(MyMainui *ui,gpointer userdata);
};

void my_mainui_show_msg(GtkToolButton *button,gpointer userdata);
GtkLayout *my_mainui_get_layout(MyMainui *self);
GtkStatusbar *my_mainui_get_statusbar(MyMainui *self);
MyMainui *my_mainui_new();
G_END_DECLS

#endif MAINUI_H_/* MAINUI_H_ */
