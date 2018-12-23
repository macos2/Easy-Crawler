/*
 * task_setting.c
 *
 *  Created on: 2017年12月25日
 *      Author: tom
 */

#include "task_setting.h"
#include "gresource.h"

typedef struct {
	GtkRadioButton *url, *source_file, *input_source, *c_filename, *a_filename;
	GtkEntry *url_entry, *xpath_entry, *prop_entry, *custom_filename;
	GtkFileChooserButton *source_filename;
	GtkCheckButton *xpath_check, *file_check, *terminal_check,
			*output_xpathprop;
} MyTaskSettingPrivate;

G_DEFINE_TYPE_WITH_CODE(MyTaskSetting, my_task_setting, GTK_TYPE_DIALOG,
		G_ADD_PRIVATE(MyTaskSetting));

static void my_task_setting_class_init(MyTaskSettingClass *klass) {
	/*gchar *template;
	gsize size;
	g_file_get_contents("task_setting.glade", &template, &size, NULL);
	gtk_widget_class_set_template(klass, g_bytes_new_static(template, size));*/
	gtk_widget_class_set_template_from_resource(klass,"/org/gtk/task_setting.glade");
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting, url);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			source_file);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			input_source);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			c_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			a_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			url_entry);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			xpath_entry);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			prop_entry);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			custom_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			source_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			xpath_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			file_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			terminal_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			output_xpathprop);
}

static void my_task_setting_init(MyTaskSetting *self) {
	gtk_widget_init_template(self);
	self->priv = my_task_setting_get_instance_private(self);
	self->set.output_filename = NULL;
	self->set.source_name = NULL;
	self->set.xpath = NULL;
}

MyTaskSetting *my_task_setting_new(task_set *set) {
	MyTaskSetting *setting = g_object_new(MY_TYPE_TASK_SETTING, NULL);
	MyTaskSettingPrivate *priv = setting->priv;
	switch (set->source) {
	case TASK_SOURCE_FILE:
		gtk_toggle_button_set_active(priv->source_file, TRUE);
		gtk_file_chooser_set_filename(priv->source_filename, set->source_name);
		break;
	case TASK_SOURCE_URL:
		gtk_toggle_button_set_active(priv->url, TRUE);
		gtk_entry_set_text(priv->url_entry, set->source_name);
		break;
	default:
		gtk_toggle_button_set_active(priv->input_source, TRUE);
		break;
	};
	gtk_toggle_button_set_active(priv->terminal_check, set->terminal_print);
	gtk_toggle_button_set_active(priv->xpath_check, set->search_xpath);
	gtk_toggle_button_set_active(priv->file_check, set->output_file);
	if (set->output_xpath) {
		gtk_toggle_button_set_active(priv->output_xpathprop, TRUE);
		gtk_entry_set_text(priv->prop_entry, set->output_xpathprop);
	}
	gtk_entry_set_text(priv->xpath_entry, set->xpath);
	gtk_toggle_button_set_active(priv->a_filename, set->auto_filename);
	gtk_toggle_button_set_active(priv->c_filename, !set->auto_filename);
	if (set->auto_filename != TRUE)
		gtk_entry_set_text(priv->custom_filename, set->output_filename);
	return setting;
}
;
void my_task_setting_get_set(MyTaskSetting *self, task_set *set) {
	MyTaskSettingPrivate *priv = self->priv;
	set->source = TASK_SOURCE_LINKER;
	if (set->source_name != NULL) {
		g_free(set->source_name);
		set->source_name = NULL;
	};
	if (set->output_filename != NULL) {
		g_free(set->output_filename);
		set->output_filename = NULL;
	};
	if (set->output_xpathprop != NULL) {
		g_free(set->output_xpathprop);
		set->output_xpathprop = NULL;
	};
	if (set->xpath != NULL) {
		g_free(set->xpath);
		set->xpath = NULL;
	};
	if (gtk_toggle_button_get_active(priv->url) == TRUE) {
		set->source = TASK_SOURCE_URL;
		set->source_name = g_strdup(gtk_entry_get_text(priv->url_entry));
	}
	if (gtk_toggle_button_get_active(priv->source_file) == TRUE) {
		set->source = TASK_SOURCE_FILE;
		set->source_name = gtk_file_chooser_get_filename(priv->source_filename);
	}
	set->search_xpath = gtk_toggle_button_get_active(priv->xpath_check);
	set->xpath = g_strdup(gtk_entry_get_text(priv->xpath_entry));
	set->output_xpath = gtk_toggle_button_get_active(priv->output_xpathprop);
	set->output_xpathprop = g_strdup(gtk_entry_get_text(priv->prop_entry));
	set->output_file = gtk_toggle_button_get_active(priv->file_check);
	set->output_filename = g_strdup(gtk_entry_get_text(priv->custom_filename));
	set->auto_filename = gtk_toggle_button_get_active(priv->a_filename);
	set->terminal_print = gtk_toggle_button_get_active(priv->terminal_check);
}
;

