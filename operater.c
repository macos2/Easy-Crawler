/*
 * operater.c
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

#include "operater.h"
#include "gresource.h"
typedef struct {
	GString *name,*log;
	GtkBox *content;
	GtkHeaderBar *headerbar;
	GtkButton *move;
	GtkLayout *parent_layout;
	GtkEventBox *headerbar_event;
	GtkTextBuffer *log_buffer;
	GtkWindow *rename_window;
	GtkEntry *rename_entry;
	gint x, y;
	GMutex *log_mutex;
} MyOperaterPrivate;

enum {
	operater_name = 1, operater_n_prop
} operater_prop;

static GParamSpec *prop[operater_n_prop] = { NULL };
gboolean headerbar_button_press_event(GtkWidget *headerbar_event,
		GdkEventButton *event, MyOperater *MyOperater);
gboolean headerbar_motion_notify_event(GtkWidget *widget, GdkEventMotion *event,
		MyOperater *self);

void add_clicked(GtkButton *button, MyOperater *self) {
	g_signal_emit_by_name(self, "add_child", NULL);
}
void close_clicked(GtkButton *button, MyOperater *self) {
	g_signal_emit_by_name(self, "close", NULL);
	gtk_widget_destroy(self);
}
void rename_clicked(GtkButton *button, MyOperater *self) {
	g_signal_emit_by_name(self, "rename", NULL);
}

void my_operater_finalize(GObject *object);
void my_operater_dispose(GObject *object);
void my_operater_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec);
void my_operater_get_property(GObject *object, guint property_id, GValue *value,
		GParamSpec *pspec);

G_DEFINE_TYPE_WITH_CODE(MyOperater, my_operater, GTK_TYPE_BOX,
		G_ADD_PRIVATE(MyOperater));

static void my_operater_class_init(MyOperaterClass *klass) {
	/*gchar *template;
	gsize size;
	g_file_get_contents("operater.glade", &template, &size, NULL);
	gtk_widget_class_set_template(klass, g_bytes_new_static(template, size));*/
	gtk_widget_class_set_template_from_resource(klass,"/org/gtk/operater.glade");
	GObjectClass *obj_class = klass;
	obj_class->set_property = my_operater_set_property;
	obj_class->get_property = my_operater_get_property;
	prop[operater_name] = g_param_spec_string("name", "name", "name", "",
			G_PARAM_READWRITE);
	g_object_class_install_properties(obj_class, operater_n_prop, prop);
	gtk_widget_class_bind_template_child_private(klass, MyOperater, content);
	gtk_widget_class_bind_template_child_private(klass, MyOperater, headerbar);
	gtk_widget_class_bind_template_child_private(klass, MyOperater,
			headerbar_event);
	gtk_widget_class_bind_template_child_private(klass, MyOperater,
			log_buffer);
	gtk_widget_class_bind_template_child_private(klass, MyOperater,
			rename_window);
	gtk_widget_class_bind_template_child_private(klass, MyOperater,
			rename_entry);
	gtk_widget_class_bind_template_callback(klass,
			headerbar_motion_notify_event);
	gtk_widget_class_bind_template_callback(klass,
			headerbar_button_press_event);
	gtk_widget_class_bind_template_callback(klass, add_clicked);
	gtk_widget_class_bind_template_callback(klass, close_clicked);
	gtk_widget_class_bind_template_callback(klass, rename_clicked);
	g_signal_new("add_child", MY_TYPE_OPERATER, G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET(MyOperaterClass, add_child), NULL, NULL, NULL,
			G_TYPE_NONE, 0, NULL);
	g_signal_new("close", MY_TYPE_OPERATER, G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET(MyOperaterClass, close), NULL, NULL, NULL,
			G_TYPE_NONE, 0, NULL);
	g_signal_new("rename", MY_TYPE_OPERATER, G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET(MyOperaterClass, rename), NULL, NULL, NULL,
			G_TYPE_NONE, 0,NULL);
}

static void my_operater_init(MyOperater *self) {
	gtk_widget_init_template(self);
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	priv->log=g_string_new("");
	priv->name=g_string_new("");
	priv->log_mutex=g_mutex_new();
	g_object_bind_property(priv->headerbar,"title",priv->rename_entry,"text",G_BINDING_BIDIRECTIONAL);
}

void my_operater_add(MyOperater *self, GtkWidget *child) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	//gtk_box_set_spacing(priv->content, gtk_box_get_spacing(priv->content) + 1);
	gtk_box_pack_start(priv->content, child, TRUE, FALSE, 0);
}
;


void my_operater_set_parent_layout(MyOperater *self, GtkLayout *layout) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	priv->parent_layout = layout;
}
;

void my_operater_set_title(MyOperater *self, gchar *title) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	if (title != NULL)
		gtk_header_bar_set_title(priv->headerbar, title);
}
;

void my_operater_set_transient_for(MyOperater *self,GtkWindow *parent_window){
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	gtk_window_set_transient_for(priv->rename_window,parent_window);
};

gchar* my_operater_get_title(MyOperater *self){
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	return gtk_header_bar_get_title(priv->headerbar);
};

void my_operater_log(MyOperater *self,gchar *log){
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	g_mutex_lock(priv->log_mutex);
	g_string_append(priv->log,log);
	gtk_text_buffer_set_text(priv->log_buffer, priv->log->str, -1);
	g_mutex_unlock(priv->log_mutex);
};

MyOperater *my_operater_new(gchar *title){
	MyOperater *op = g_object_new(MY_TYPE_OPERATER, NULL);
	my_operater_set_title(op, title);
	return op;
};

gboolean headerbar_motion_notify_event(GtkWidget *widget, GdkEventMotion *event,
		MyOperater *self) {
	GtkAllocation allocation;
	gint x = 0, y = 0;
	MyOperaterPrivate *priv = my_operater_get_instance_private(self);
	if ((event->state & GDK_BUTTON1_MASK) != 0) {
		gtk_widget_get_allocation(self, &allocation);
		if (allocation.x >= 0 || event->x >= 0)
			x = event->x - priv->x;
		if (allocation.y >= 0 || event->y >= 0)
			y = event->y - priv->y;
		gtk_layout_move(priv->parent_layout, self, x + allocation.x,
				y + allocation.y);
		gtk_widget_queue_draw(priv->parent_layout);
	}
	return G_SOURCE_CONTINUE;
}
;

gboolean headerbar_button_press_event(GtkWidget *headerbar_event,
		GdkEventButton *event, MyOperater *MyOperater) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(MyOperater);
	priv->x = event->x;
	priv->y = event->y;
	return G_SOURCE_CONTINUE;
}
;

void my_operater_finalize(GObject *object) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(object);
	G_OBJECT_CLASS(my_operater_parent_class)->finalize(object);
}
;

void my_operater_dispose(GObject *object) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(object);
	g_clear_object(&priv->content);
	g_clear_object(&priv->headerbar);
	g_clear_object(&priv->move);
	g_string_free(priv->name, TRUE);
	G_OBJECT_CLASS(my_operater_parent_class)->dispose(object);
}
;
void my_operater_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(object);
	switch (property_id) {
	case operater_name:
		gtk_header_bar_set_title(priv->headerbar, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	};
}
;
void my_operater_get_property(GObject *object, guint property_id, GValue *value,
		GParamSpec *pspec) {
	MyOperaterPrivate *priv = my_operater_get_instance_private(object);
	switch (property_id) {
	case operater_name:
		g_value_set_string(value, gtk_header_bar_get_title(priv->headerbar));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	};
}
;
