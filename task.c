/*
 * task.c
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */
#include "task.h"

typedef struct {
	GString *name;
	GtkImage *state, *next, *icon;
	GtkLabel *label;
	GtkEventBox *state_event,*next_event;
} MyTaskPrivate;

void content_clicked(GtkButton *button, MyTask *self);
void del_clicked(GtkButton *button, MyTask *self);
void link_del_clicked(GtkButton *button, MyTask *self);
void my_task_finalize(GObject *object);
void my_task_dispose(GObject *object);
void my_task_get_property(GObject *object, guint property_id, GValue *value,
		GParamSpec *pspec);
void my_task_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec);
G_DEFINE_TYPE_WITH_CODE(MyTask, my_task, GTK_TYPE_BOX, G_ADD_PRIVATE(MyTask));

static GParamSpec *prop[task_n_prop] = { NULL, };

static void my_task_class_init(MyTaskClass *klass) {
	/*gchar* template;
	gsize size;
	g_file_get_contents("task.glade", &template, &size,
	NULL);
	gtk_widget_class_set_template(klass, g_bytes_new_take(template, size));*/
	gtk_widget_class_set_template_from_resource(klass,"/org/gtk/task.glade");
	GObjectClass *obj_class = klass;
	obj_class->set_property = my_task_set_property;
	obj_class->get_property = my_task_get_property;
	prop[task_icon] = g_param_spec_string("icon", "icon", "icon", "",
			G_PARAM_READWRITE);
	prop[task_state] = g_param_spec_string("state", "state", "state",
			"", G_PARAM_READWRITE);
	prop[task_next] = g_param_spec_boolean("next", "next", "next", FALSE,
			G_PARAM_READWRITE);
	prop[task_label] = g_param_spec_string("label", "label", "label", "label",
			G_PARAM_READWRITE);
	prop[task_name] = g_param_spec_string("name", "name", "name", "name",
			G_PARAM_READWRITE);
	g_object_class_install_properties(obj_class, task_n_prop, prop);
	g_signal_new("remove_task",MY_TYPE_TASK,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyTaskClass,remove_task),NULL,NULL,NULL,G_TYPE_NONE,1,GTK_TYPE_WIDGET,NULL);
	g_signal_new("content_clicked",MY_TYPE_TASK,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyTaskClass,content_clicked),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	g_signal_new("link_del",MY_TYPE_TASK,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyTaskClass,link_del),NULL,NULL,NULL,G_TYPE_NONE,0,NULL);
	gtk_widget_class_bind_template_child_private(klass, MyTask, icon);
	gtk_widget_class_bind_template_child_private(klass, MyTask, next);
	gtk_widget_class_bind_template_child_private(klass, MyTask, state);
	gtk_widget_class_bind_template_child_private(klass, MyTask, label);
	gtk_widget_class_bind_template_child_private(klass, MyTask, state_event);
	gtk_widget_class_bind_template_child_private(klass, MyTask, next_event);
	gtk_widget_class_bind_template_callback(klass, content_clicked);
	gtk_widget_class_bind_template_callback(klass, del_clicked);
	gtk_widget_class_bind_template_callback(klass, link_del_clicked);
}
;

static void my_task_init(MyTask *self) {
	gtk_widget_init_template(self);
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	GtkWidget *toplevel = gtk_widget_get_toplevel(self);
	gtk_label_set_text(priv->label, "");
}
;


MyTask *my_task_new(){
	MyTask *ta = g_object_new(MY_TYPE_TASK, NULL);
	return ta;
};

GtkImage *my_task_get_next_icon(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->next;
};
GtkImage *my_task_get_state_icon(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->state;
};

GtkImage *my_task_get_icon(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->icon;
};
GtkLabel *my_task_get_label(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->label;
};


GtkEventBox *my_task_get_next_event(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->next_event;
};
GtkEventBox *my_task_get_state_event(MyTask *self){
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	return priv->state_event;
};


void content_clicked(GtkButton *button, MyTask *self) {
	MyTaskPrivate *priv = my_task_get_instance_private(self);
	g_signal_emit_by_name(self,"content_clicked",NULL);
}
;

void del_clicked(GtkButton *button, MyTask *self) {
	g_signal_emit_by_name(self,"remove_task",gtk_widget_get_parent(self),NULL);
	gtk_widget_destroy(self);
}
;

void link_del_clicked(GtkButton *button, MyTask *self){
	g_signal_emit_by_name(self,"link_del",gtk_widget_get_parent(self),NULL);
};
void my_task_dispose(GObject *object){
	MyTaskPrivate *priv = my_task_get_instance_private(object);
	g_clear_object(&priv->label);
	g_clear_object(&priv->icon);
	g_clear_object(&priv->next);
	g_clear_object(&priv->state);
	g_string_free(priv->name,TRUE);
	G_OBJECT_CLASS(my_task_parent_class)->dispose(object);
};

void my_task_finalize(GObject *object) {
	MyTaskPrivate *priv = my_task_get_instance_private(object);

	G_OBJECT_CLASS(my_task_parent_class)->finalize(object);
}
;

void my_task_get_property(GObject *object, guint property_id, GValue *value,
		GParamSpec *pspec) {
	MyTaskPrivate *priv = my_task_get_instance_private(object);
	gchar *img_name;
	switch (property_id) {
	case task_name:
		g_value_set_string(value, priv->name->str);
		break;
	case task_icon:
		gtk_image_get_icon_name(priv->icon, &img_name, GTK_ICON_SIZE_BUTTON);
		g_value_set_string(value, img_name);
		break;
	case task_label:
		g_value_set_string(value, gtk_label_get_label(priv->label));
		break;
	case task_next:
		gtk_image_get_icon_name(priv->next, &img_name, GTK_ICON_SIZE_BUTTON);
		if (g_strcmp0("gtk-discard", img_name) == 0) {
			g_value_set_boolean(value, FALSE);
		} else {
			g_value_set_boolean(value, TRUE);
		}
		;
		break;
	case task_state:
		gtk_image_get_icon_name(priv->icon, &img_name, GTK_ICON_SIZE_BUTTON);
		g_value_set_string(value, img_name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	};

}
;
void my_task_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec) {
	MyTaskPrivate *priv = my_task_get_instance_private(object);
	switch (property_id) {
	case task_name:
		g_string_printf(priv->name, "%s", g_value_get_string(value));
		break;
	case task_icon:
		gtk_image_set_from_icon_name(priv->icon, g_value_get_string(value),
				GTK_ICON_SIZE_BUTTON);
		break;
	case task_label:
		gtk_label_set_label(priv->label, g_value_get_string(value));
		break;
	case task_next:
		gtk_image_set_from_icon_name(priv->next, "",
				GTK_ICON_SIZE_BUTTON);
		if (g_value_get_boolean(value))
			gtk_image_set_from_icon_name(priv->next,"go-next",
					GTK_ICON_SIZE_BUTTON);
		break;
	case task_state:
		gtk_image_set_from_icon_name(priv->state, g_value_get_string(value),
				GTK_ICON_SIZE_BUTTON);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	};

}
;
