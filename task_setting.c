/*
 * task_setting.c
 *
 *  Created on: 2017年12月25日
 *      Author: tom
 */

#include "task_setting.h"
#include "gresource.h"

typedef struct {
	GtkRadioButton *url, *source_file, *input_source;
	GtkEntry *url_entry, *xpath_entry,*fmt_filename;
	GtkFileChooserButton *source_filename;
	GtkCheckButton *xpath_check, *file_check, *terminal_check,
			*output_modify;
	GtkEntryBuffer *fmt_output,*regex_pattern;
	GtkTextBuffer *regex_test_text,*result_text;
} MyTaskSettingPrivate;

G_DEFINE_TYPE_WITH_CODE(MyTaskSetting, my_task_setting, GTK_TYPE_DIALOG,
		G_ADD_PRIVATE(MyTaskSetting));

void my_task_setting_test(GtkButton *button,MyTaskSetting *setting);

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
			fmt_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			url_entry);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			xpath_entry);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			source_filename);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			xpath_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			file_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			terminal_check);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			output_modify);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			fmt_output);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			regex_pattern);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			regex_test_text);
	gtk_widget_class_bind_template_child_private(klass, MyTaskSetting,
			result_text);
	gtk_widget_class_bind_template_callback(klass,my_task_setting_test);
	g_signal_new("test",MY_TYPE_TASK_SETTING,G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET(MyTaskSettingClass,test),NULL,NULL,NULL,G_TYPE_POINTER,3,G_TYPE_POINTER,G_TYPE_POINTER,G_TYPE_POINTER,NULL);
}

static void my_task_setting_init(MyTaskSetting *self) {
	gtk_widget_init_template(self);
	self->priv = my_task_setting_get_instance_private(self);
	self->set.fmt_filename = NULL;
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
	if (set->output_modify) {
		gtk_toggle_button_set_active(priv->output_modify, TRUE);
	}
	gtk_entry_set_text(priv->xpath_entry, set->xpath);
	gtk_entry_set_text(priv->fmt_filename,set->fmt_filename);
	gtk_entry_buffer_set_text(priv->regex_pattern,set->regex_pattern,-1);
	gtk_entry_buffer_set_text(priv->fmt_output,set->fmt_output,-1);
	gtk_text_buffer_set_text(priv->regex_test_text,set->regex_test_text,-1);
	return setting;
}
;

void my_task_setting_test(GtkButton *button,MyTaskSetting *setting){
	guint i;
	GtkTextIter start,end;
	gchar *regex_pattern,*output_fmt,*eval_content,*t;
	GArray *result;
	GString *temp=g_string_new("");
	MyTaskSettingPrivate *priv=my_task_setting_get_instance_private(setting);
	regex_pattern=gtk_entry_buffer_get_text(priv->regex_pattern);
	output_fmt=gtk_entry_buffer_get_text(priv->fmt_output);
	gtk_text_buffer_get_start_iter(priv->regex_test_text,&start);
	gtk_text_buffer_get_end_iter(priv->regex_test_text,&end);
	eval_content=gtk_text_buffer_get_text(priv->regex_test_text,&start,&end,TRUE);
	g_signal_emit_by_name(setting,"test",regex_pattern,output_fmt,eval_content,&result);
	for(i=0;i<result->len;i++){
		t=g_array_index(result,gpointer,i);
		g_string_append_printf(temp,"\"%s\"\n",t);
	}
	gtk_text_buffer_set_text(priv->result_text,temp->str,-1);
	g_string_free(temp,TRUE);
	g_free(eval_content);
	g_ptr_array_set_free_func(result, g_free);
	g_array_free(result,TRUE);
};

void my_task_setting_get_set(MyTaskSetting *self, task_set *set) {
	GtkTextIter start,end;
	MyTaskSettingPrivate *priv = self->priv;
	set->source = TASK_SOURCE_LINKER;
	if (set->source_name != NULL) {
		g_free(set->source_name);
		set->source_name = NULL;
	};
	if (set->fmt_filename != NULL) {
		g_free(set->fmt_filename);
		set->fmt_filename = NULL;
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
	set->output_modify = gtk_toggle_button_get_active(priv->output_modify);
	set->output_file = gtk_toggle_button_get_active(priv->file_check);
	set->terminal_print = gtk_toggle_button_get_active(priv->terminal_check);
	set->fmt_filename=g_strdup(gtk_entry_get_text(priv->fmt_filename));
	set->regex_pattern=g_strdup(gtk_entry_buffer_get_text(priv->regex_pattern));
	set->fmt_output=g_strdup(gtk_entry_buffer_get_text(priv->fmt_output));
	gtk_text_buffer_get_start_iter(priv->regex_test_text,&start);
	gtk_text_buffer_get_end_iter(priv->regex_test_text,&end);
	set->regex_test_text=gtk_text_buffer_get_text(priv->regex_test_text,&start,&end,TRUE);
}
;


