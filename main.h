/*
 * main.h
 *
 *  Created on: 2017年12月26日
 *      Author: tom
 */

#ifndef MAINFUN_C_
#define MAINFUN_C_
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <libsoup-2.4/libsoup/soup.h>
#include <libxml2/libxml/xpath.h>
#include <libxml2/libxml/HTMLparser.h>
#include "mainui.h"
#include "operater.h"
#include "task.h"
#include "task_setting.h"
#include "task_message.h"
#include "mainui_setting.h"
#include "MySoupDl.h"
//#include "MyCurl.h"

typedef struct {
	MyOperater *operater;
	GList *task;
	GHashTable *task_set, *oj;
	MyMainui *ui;
	GString *log;
} op_data;

SoupSession *session;
GHashTable *operater_to_opdata;
GHashTable *task_to_opdata;
GHashTable *task_to_linklist;
GHashTable *log_task_href;
gchar *OUTPUT_DIR;
gchar *COOKIE_FILE;
gint *MAX_THREAD;
GIOChannel *log_file;
GMutex *log_mutex, *log_task_href_mutex;
GMutex notify_download_mutex;
gboolean stop_thread;
MyDownloadUi *down_ui;
//MyCurl *mycurl;
MySoupDl *mysoupdl;
GtkWindow *down_win;
GRegex *fmt_filename_date, *fmt_filename_time, *fmt_filename_title,
		*fmt_filename_org, *fmt_filename_uri, *meta_charset;
GList *notify_download_list, *finish_download_list;
guint runing_count = 0;
GAsyncQueue *process_queue;
GtkListStore *additional_head_request;
static gint task_id = 0;
static gint operater_id = 0;
static gchar *size_unit[] = { "Byte", "KiB", "MiB", "GiB", "TiB" };

gboolean task_source(MyTaskMessage *task_msg);
GArray* task_xpath_content_fmt(const MyTaskMessage *task_msg, const gchar *fmt,
		const GArray *content);
GArray* task_regex_match(const gchar *regex_pattern, const gchar *eval_content);

void runing_count_decrease() {
	if (runing_count > 0)
		runing_count--;
}
/*
 void format_size(gdouble *size, gchar *i) {
 *i = 0;
 while (*size > 1024) {
 *size = *size / 1024.;
 *i = *i + 1;
 if (*i >= 4)
 break;
 }
 }

 gchar *format_size_to_str(gdouble size) {
 gchar i;
 format_size(&size, &i);
 return g_strdup_printf("%3.2f %s", size, size_unit[i]);
 }

 gchar *format_dlsize_size(gint64 dlsize, gint64 content_size) {
 gchar i, j;
 gdouble s1 = dlsize, s2 = content_size;
 format_size(&s1, &i);
 format_size(&s2, &j);
 return g_strdup_printf("%3.2f %s/%3.2f %s", s1, size_unit[i], s2,
 size_unit[j]);
 }
 */

void session_authenticate(SoupSession *session, SoupMessage *msg,
		SoupAuth *auth, gboolean retrying, gpointer user_data) {
	GtkDialog *dialog = gtk_dialog_new();
	gtk_dialog_add_button(dialog, "Ok", GTK_RESPONSE_OK);
	gtk_dialog_add_button(dialog, "Cancel", GTK_RESPONSE_CANCEL);
	GtkBox *content = gtk_dialog_get_content_area(dialog);
	GtkLabel *label = gtk_label_new("Authenticate required");
	GtkEntry *name = gtk_entry_new();
	GtkEntry *password = gtk_entry_new();
	gtk_entry_set_placeholder_text(name, "Name");
	gtk_entry_set_placeholder_text(password, "PassWord");
	gtk_entry_set_visibility(password, FALSE);
	gtk_box_pack_start(content, label, TRUE, FALSE, 2);
	gtk_box_pack_start(content, name, TRUE, FALSE, 2);
	gtk_box_pack_start(content, password, TRUE, FALSE, 2);
	gtk_widget_show_all(content);
	if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
		soup_auth_authenticate(auth, gtk_entry_get_text(name),
				gtk_entry_get_text(password));
	} else {
		soup_session_cancel_message(session, msg, SOUP_STATUS_CANCELLED);
	};
	gtk_widget_destroy(dialog);
}
;

op_data* operater_get_opdata(MyOperater *op) {
	return g_hash_table_lookup(operater_to_opdata, op);
}
;
op_data* task_get_opdata(MyTask *task) {
	return g_hash_table_lookup(task_to_opdata, task);
}

task_set* task_get_set(MyTask *task) {
	op_data *data = g_hash_table_lookup(task_to_opdata, task);
	return g_hash_table_lookup(data->task_set, task);
}

GList* task_get_linklist(MyTask *task) {
	return g_hash_table_lookup(task_to_linklist, task);
}

void task_remove_task(MyTask *self, GtkWidget *parent, gpointer userdata) {
	GtkBox *box = parent;
	op_data *data = userdata;
	task_set *set;
	GList *list = g_hash_table_lookup(task_to_linklist, self);
	gtk_container_remove(box, self);
	gtk_box_set_spacing(box, gtk_box_get_spacing(box) - 1);
	data->task = g_list_remove(data->task, self);
	set = g_hash_table_lookup(data->task_set, self);
	g_free(set->fmt_filename);
	g_free(set->source_name);
	g_free(set->xpath);
	g_free(set);
	g_hash_table_remove(data->task_set, self);
	g_hash_table_remove(task_to_opdata, self);
	if (list != NULL)
		g_list_free(list);
	g_hash_table_remove(task_to_linklist, self);
	gchar *tmp = g_strdup_printf("task remove %u\n", g_list_length(data->task));
	my_operater_log(data->operater, tmp);
	g_free(tmp);
}
;

void task_drag_data_get(GtkWidget *widget, GdkDragContext *context,
		GtkSelectionData *data, guint info, guint time, gpointer user_data) {
	switch (info) {
	case 0:
		gtk_selection_data_set(data, gdk_atom_intern("Task Link", TRUE), 0,
				&user_data, sizeof(gpointer));
		break;
	default:
		break;
	}
}
;

void task_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x,
		gint y, GtkSelectionData *data, guint info, guint time,
		gpointer user_data) {
	gpointer *receive;
	GList *list;
	op_data *src, *des;
	switch (info) {
	case 0:
		receive = gtk_selection_data_get_data(data);
		if (*receive == user_data)
			break; //不允许同一个task连接
		src = g_hash_table_lookup(task_to_opdata, *receive);
		des = g_hash_table_lookup(task_to_opdata, user_data);
		/*	if (src == des)
		 break; //不允许同一个operater连接*/
		list = g_hash_table_lookup(task_to_linklist, *receive);
		if (g_list_find(list, *receive) != NULL)
			break; //链接已存在
		list = g_list_append(list, user_data);
		g_hash_table_insert(task_to_linklist, *receive, list);
		op_data *data = task_get_opdata(user_data);
		gtk_widget_queue_draw(my_mainui_get_layout(data->ui));
		break;
	default:
		break;
	}
}
;
void task_update_status(MyTask *self, task_set *set) {
	gchar *str = NULL;
	if (set->search_xpath||set->output_modify) {
		gtk_image_set_from_icon_name(my_task_get_next_icon(self),
				"go-next-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_connect(my_task_get_next_event(self), "drag-data-get",
				task_drag_data_get, self);
	} else {
		gtk_image_set_from_icon_name(my_task_get_next_icon(self), "",
				GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_handlers_disconnect_by_func(my_task_get_next_event(self),
				task_drag_data_get, self);
		g_hash_table_remove(task_to_linklist, self);
	};
	switch (set->source) {
	case TASK_SOURCE_FILE:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"text-x-generic-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		break;
	case TASK_SOURCE_URL:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"network-transmit-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		break;
	default:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"go-next-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		break;
	}
	if (set->search_xpath == TRUE) {
		gtk_image_set_from_icon_name(my_task_get_icon(self),
				"system-search-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		str = g_strdup_printf("%s", set->xpath);
		gtk_label_set_text(my_task_get_label(self), str);
		g_free(str);
	} else if (set->output_file == TRUE) {
		gtk_image_set_from_icon_name(my_task_get_icon(self),
				"folder-download-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		str = g_strdup_printf("%s", set->fmt_filename);
		gtk_label_set_text(my_task_get_label(self), str);
		g_free(str);
	} else {
		gtk_image_set_from_icon_name(my_task_get_icon(self),
				"emblem-synchronizing-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_label_set_text(my_task_get_label(self), "Load Link Content");
	}
}
;
GArray* task_content_test(MyTaskSetting *setting, gchar *regex_pattren,
		gchar *output_fmt, gchar *eval_content, void *user_data) {
	GArray *regex_arr, *fmt_arr;
	regex_arr = task_regex_match(regex_pattren, eval_content);
	fmt_arr = task_xpath_content_fmt(NULL, output_fmt, regex_arr);
	g_ptr_array_set_free_func(regex_arr, g_free);
	g_array_free(regex_arr, TRUE);
	return fmt_arr;
}
;

void task_content_clicked(MyTask *self, gpointer userdata) {
	op_data *data = userdata;
	task_set *set = g_hash_table_lookup(data->task_set, self);
	MyTaskSetting *setting = my_task_setting_new(set);
	gtk_window_set_transient_for(setting, data->ui);
	g_signal_connect(setting, "test", task_content_test, NULL);
	if (gtk_dialog_run(setting) == GTK_RESPONSE_OK)
		my_task_setting_get_set(setting, set); //保存设置
	gtk_widget_destroy(setting);
	task_update_status(self, set);
}
;

void task_link_del_clicked(MyTask *self, gpointer userdata) {
	op_data *data = userdata;
	g_hash_table_remove(task_to_linklist, self);
	gtk_widget_queue_draw(data->ui);
}

MyTask* operater_add_task(MyOperater *self, gpointer userdata) {
	op_data *data = userdata;
	MyTask *task = my_task_new();
	task_set *set = g_malloc0(sizeof(task_set));
	my_operater_add(self, task);
	data->task = g_list_append(data->task, task);
	g_hash_table_insert(data->task_set, task, set);
	set->source_name = g_strdup("");
	set->xpath = g_strdup("//a/@href");
	set->fmt_filename = g_strdup("%f");
	set->source = TASK_SOURCE_LINKER;
	set->regex_pattern = g_strdup(".+");
	set->fmt_output = g_strdup("%f");
	set->regex_test_text = g_strdup("");
	set->search_xpath = TRUE;
	//set->link_wait = TRUE;
	set->same_uri_skip = TRUE;
	g_signal_connect(task, "remove_task", task_remove_task, data);
	g_signal_connect(task, "content_clicked", task_content_clicked, data);
	g_signal_connect(task, "link_del", task_link_del_clicked, data);
	gchar *tmp = g_strdup_printf("add task %u\n", g_list_length(data->task));
	my_operater_log(self, tmp);
	g_free(tmp);
	GtkTargetEntry targetentry[1];
	targetentry[0].flags = GTK_TARGET_SAME_APP;
	targetentry[0].info = 0;
	targetentry[0].target = g_strdup("Link Data");
	gtk_drag_dest_set(my_task_get_state_event(task), GTK_DEST_DEFAULT_ALL,
			targetentry, 1, GDK_ACTION_MOVE);
	gtk_drag_source_set(my_task_get_next_event(task), GDK_BUTTON1_MASK,
			targetentry, 1, GDK_ACTION_MOVE);
	g_signal_connect(my_task_get_state_event(task), "drag-data-received",
			task_drag_data_received, task);
	g_hash_table_insert(task_to_opdata, task, data);
	task_update_status(task, set);
	return task;
}
;

MyTask* operater_add_task_with_set(MyOperater *self, op_data *data,
		task_set *task_set) {
	MyTask *task = my_task_new();
	my_operater_add(self, task);
	data->task = g_list_append(data->task, task);
	g_hash_table_insert(data->task_set, task, task_set);
	g_signal_connect(task, "remove_task", task_remove_task, data);
	g_signal_connect(task, "content_clicked", task_content_clicked, data);
	g_signal_connect(task, "link_del", task_link_del_clicked, data);
	gchar *tmp = g_strdup_printf("add task %u\n", g_list_length(data->task));
	my_operater_log(self, tmp);
	g_free(tmp);
	GtkTargetEntry targetentry[1];
	targetentry[0].flags = GTK_TARGET_SAME_APP;
	targetentry[0].info = 0;
	targetentry[0].target = g_strdup("Link Data");
	gtk_drag_dest_set(my_task_get_state_event(task), GTK_DEST_DEFAULT_ALL,
			targetentry, 1, GDK_ACTION_MOVE);
	gtk_drag_source_set(my_task_get_next_event(task), GDK_BUTTON1_MASK,
			targetentry, 1, GDK_ACTION_MOVE);
	g_signal_connect(my_task_get_state_event(task), "drag-data-received",
			task_drag_data_received, task);
	g_hash_table_insert(task_to_opdata, task, data);
	task_update_status(task, task_set);
	return task;
}
;

void operater_close(MyOperater *self, gpointer userdata) {
	op_data *data = userdata;
	if (data->task != NULL)
		while (data->task != NULL) {
			data->task = g_list_first(data->task);
			task_remove_task(data->task->data, self, data);
		}
	g_hash_table_unref(data->task_set);
	g_list_free(data->task);
	g_free(data);
	g_hash_table_remove(operater_to_opdata, self);
	gtk_widget_destroy(self);
}
;
void main_ui_clear_operater(MyMainui *ui, gpointer userdata) {
	op_data *data;
	GList *operater_list = g_hash_table_get_keys(operater_to_opdata);
	while (operater_list != NULL) {
		data = g_hash_table_lookup(operater_to_opdata, operater_list->data);
		operater_close(operater_list->data, data);
		if (operater_list->next == NULL) {
			g_list_free(operater_list);
			operater_list = NULL;
		} else {
			operater_list = operater_list->next;
		};
	};
}
;

MyOperater* main_ui_add(MyMainui *ui, gpointer userdata) {
	op_data *data = g_malloc(sizeof(op_data));
	gchar *str = g_strdup_printf("%d", operater_id++);
	MyOperater *op = my_operater_new(str);
	g_free(str);
	GtkLayout *layout = my_mainui_get_layout(ui);
	my_operater_set_parent_layout(op, layout);
	my_operater_set_transient_for(op, ui);
	gtk_layout_put(layout, op, 0, 0);
	data->operater = op;
	data->task = NULL;
	data->task_set = g_hash_table_new(g_direct_hash, g_direct_equal);
	data->ui = ui;
	g_hash_table_insert(operater_to_opdata, op, data);
	g_signal_connect(op, "add_child", operater_add_task, data);
	g_signal_connect(op, "close", operater_close, data);
	if (userdata == NULL)
		operater_add_task(op, data);
	return op;
}
;

void main_ui_save_string(gchar *str, GOutputStream *out) {
	size_t i;
	if (str == NULL) {
		i = 0;
		g_output_stream_write(out, &i, sizeof(size_t), NULL, NULL);
	} else {
		i = strlen(str);
		g_output_stream_write(out, &i, sizeof(size_t), NULL, NULL);
		g_output_stream_write(out, str, i, NULL, NULL);
	}
}

gchar* main_ui_read_string(GInputStream *in) {
	size_t i;
	g_input_stream_read(in, &i, sizeof(size_t), NULL, NULL);
	if (i == 0)
		return g_strdup("");
	gchar *str = g_malloc0(i + 1);
	g_input_stream_read(in, str, i, NULL, NULL);
	return str;
}

void main_ui_save_list(GList *list, GOutputStream *out) {
	guint length = 0;
	length = g_list_length(list);
	g_output_stream_write(out, &length, sizeof(guint), NULL, NULL);
	if (length > 0) {
		GList *l = g_list_first(list);
		while (l != NULL) {
			g_output_stream_write(out, &l->data, sizeof(gpointer), NULL, NULL);
			l = l->next;
		}
	};
}

GList* main_ui_read_list(GList *list, GInputStream *in) {
	guint i = 0;
	guint length = 0;
	gpointer p;
	GList *tem = list;
	g_input_stream_read(in, &length, sizeof(guint), NULL, NULL);
	for (i = 0; i < length; i++) {
		g_input_stream_read(in, &p, sizeof(gpointer), NULL, NULL);
		tem = g_list_append(tem, p);
	};
	return tem;
}

void task_set_read_from_file(task_set *set, GInputStream *in) {
	g_input_stream_read(in, &(set->output_file), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->output_modify), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->search_xpath), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->terminal_print), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->title_insist), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->nonnull_break), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->link_wait), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->same_uri_skip), sizeof(gboolean), NULL,
	NULL);
	//保留字段,跳过读取
	gint i;
	gboolean r;
	for (i = 3; i != 0; i--) {
		g_input_stream_read(in, &r, sizeof(gboolean), NULL,
		NULL);
	}

	set->fmt_filename = main_ui_read_string(in);
	set->fmt_output = main_ui_read_string(in);
	set->regex_pattern = main_ui_read_string(in);
	set->regex_test_text = main_ui_read_string(in);
	set->source_name = main_ui_read_string(in);
	set->xpath = main_ui_read_string(in);
	g_input_stream_read(in, &(set->source), sizeof(guint), NULL, NULL);
}
;

void task_set_save_to_file(task_set *set, GOutputStream *out) {
	g_output_stream_write(out, &(set->output_file), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->output_modify), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->search_xpath), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->terminal_print), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->title_insist), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->nonnull_break), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->link_wait), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->same_uri_skip), sizeof(gboolean), NULL,
	NULL);
	//保留字段,方便日后扩展
	gint i = 3;
	gboolean r[i];
	g_output_stream_write(out, r, sizeof(gboolean) * i, NULL,
	NULL);

	main_ui_save_string(set->fmt_filename, out);
	main_ui_save_string(set->fmt_output, out);
	main_ui_save_string(set->regex_pattern, out);
	main_ui_save_string(set->regex_test_text, out);
	main_ui_save_string(set->source_name, out);
	main_ui_save_string(set->xpath, out);
	g_output_stream_write(out, &(set->source), sizeof(guint), NULL, NULL);
}
;

void update_task_address(gpointer data, GHashTable *task_id_table) {
	data = g_hash_table_lookup(task_id_table, data);
}
;

void main_ui_save_additional_header_request(GOutputStream *out) {
	GtkTreeIter iter;
	gchar *name, *value;
	gint n_row = gtk_tree_model_iter_n_children(additional_head_request, NULL);
	gtk_tree_model_get_iter_first(additional_head_request, &iter);
	g_output_stream_write(out, &n_row, sizeof(gint), NULL, NULL);
	while (n_row > 0) {
		gtk_tree_model_get(additional_head_request, &iter, 0, &name, 1, &value,
				-1);
		main_ui_save_string(name, out);
		main_ui_save_string(value, out);
		g_free(name);
		g_free(value);
		gtk_tree_model_iter_next(additional_head_request, &iter);
		n_row--;
	}

}
;

void main_ui_load_additional_header_request(GInputStream *in) {
	gint n_row = 0;
	GtkTreeIter iter;
	gchar *name, *value;
	g_input_stream_read(in, &n_row, sizeof(gint), NULL, NULL);
	if (n_row > 0) {
		gtk_list_store_clear(additional_head_request);
		while (n_row > 0) {
			name = main_ui_read_string(in);
			value = main_ui_read_string(in);
			gtk_list_store_append(additional_head_request, &iter);
			gtk_list_store_set(additional_head_request, &iter, 0, name, 1,
					value, -1);
			g_free(name);
			g_free(value);
			n_row--;
		}
	}

}
;

void main_ui_save(MyMainui *ui, gpointer userdata) {
	GtkDialog *dialog = gtk_file_chooser_dialog_new("Open", ui,
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.ecc");
	gtk_file_filter_set_name(filter, "Easy Crawler Data File(*.ecc)");
	gtk_file_chooser_add_filter(dialog, filter);
	gint i = gtk_dialog_run(dialog);
	gchar *filename, *temp;
	GIOStream *io;
	GOutputStream *output;
	GList *list, *task_list;
	op_data *data;
	task_set *set;
	MyTask *task;
	MyOperater *operater;
	GtkAllocation allocation;
	guint task_count, operater_count, link_count, length;
	switch (i) {
	case GTK_RESPONSE_OK:
		temp = gtk_file_chooser_get_filename(dialog);
		g_print("Save file:%s.ecc\n", temp);
		if (g_strrstr(temp, ".ecc") == NULL) {
			filename = g_strdup_printf("%s.ecc", temp);
		} else {
			filename = g_strdup_printf("%s", temp);
		}
		g_free(temp);
		GFile *file = g_file_new_for_path(filename);
		g_unlink(filename);
		io = g_file_create_readwrite(file, G_FILE_CREATE_REPLACE_DESTINATION,
		NULL, NULL);
		output = g_io_stream_get_output_stream(io);
		task_count = g_hash_table_size(task_to_opdata);
		operater_count = g_hash_table_size(operater_to_opdata);
		link_count = g_hash_table_size(task_to_linklist);
		//保存任务信息
		g_output_stream_write(output, &task_count, sizeof(guint), NULL, NULL);
		list = g_hash_table_get_keys(task_to_opdata);
		list = g_list_first(list);
		while (list != NULL) {
			task = list->data;
			set = task_get_set(task);
			g_output_stream_write(output, &task, sizeof(gpointer), NULL, NULL);
			task_set_save_to_file(set, output);
			if (list->next == NULL) {
				g_list_free(list);
				list = NULL;
			} else {
				list = list->next;
			}
		}
		;

		//保存操作主体信息
		g_output_stream_write(output, &operater_count, sizeof(guint), NULL,
		NULL);
		list = g_hash_table_get_keys(operater_to_opdata);
		list = g_list_first(list);
		while (list != NULL) {
			operater = list->data;
			data = operater_get_opdata(operater);
			gtk_widget_get_allocation(operater, &allocation);
			g_output_stream_write(output, &operater, sizeof(gpointer), NULL,
			NULL);
			main_ui_save_string(my_operater_get_title(operater), output);
			g_output_stream_write(output, &allocation, sizeof(GtkAllocation),
			NULL, NULL);
			task_list = data->task;
			task_list = g_list_first(task_list);
			main_ui_save_list(task_list, output);
			if (list->next == NULL) {
				g_list_free(list);
				list = NULL;
			} else {
				list = list->next;
			}
		}

		//保存任务连接；
		g_output_stream_write(output, &link_count, sizeof(guint), NULL,
		NULL);
		list = g_hash_table_get_keys(task_to_linklist);
		while (list != NULL) {
			g_output_stream_write(output, &list->data, sizeof(gpointer), NULL,
			NULL);
			main_ui_save_list(g_hash_table_lookup(task_to_linklist, list->data),
					output);
			if (list->next == NULL) {
				g_list_free(list);
				list = NULL;
			} else {
				list = list->next;
			}
		}
		;
		//保存外部cookies 文件路径
		main_ui_save_string(COOKIE_FILE, output);
		//保存附加请求头值
		main_ui_save_additional_header_request(output);

		g_io_stream_close(io, NULL, NULL);
		g_free(filename);
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);
}

void main_ui_open(MyMainui *ui, gpointer userdata) {
	GtkDialog *dialog = gtk_file_chooser_dialog_new("Open", ui,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.ecc");
	gtk_file_filter_set_name(filter, "Easy Crawler Data File(*.ecc)");
	gtk_file_chooser_add_filter(dialog, filter);
	guint i = gtk_dialog_run(dialog), j;
	guint task_count, operater_count, link_count, legth;
	gchar *filename, *title;
	GIOStream *io;
	GInputStream *in;
	GFile *file;
	GHashTable *task_table, *task_id_table;
	GtkAllocation allocation;
	GList *tem_list = NULL, *tem_list2 = NULL;
	MyTask *task;
	switch (i) {
	case GTK_RESPONSE_OK:
		operater_id = 0;
		main_ui_clear_operater(ui, userdata);
		filename = gtk_file_chooser_get_filename(dialog);
		file = g_file_new_for_path(filename);
		io = g_file_open_readwrite(file, NULL, NULL);
		in = g_io_stream_get_input_stream(io);
		gpointer p;
		//读取Task信息；
		g_input_stream_read(in, &task_count, sizeof(guint), NULL, NULL);
		task_table = g_hash_table_new(g_direct_hash, g_direct_equal);
		task_set *task_setting;
		for (i = 0; i < task_count; i++) {
			task_setting = g_malloc0(sizeof(task_set));
			g_input_stream_read(in, &p, sizeof(gpointer), NULL, NULL);
			task_set_read_from_file(task_setting, in);
			g_hash_table_insert(task_table, p, task_setting);
		}
		;
		//读取operater信息；
		g_input_stream_read(in, &operater_count, sizeof(guint), NULL, NULL);
		task_id_table = g_hash_table_new(g_direct_hash, g_direct_equal);
		op_data *opdata;
		for (i = 0; i < operater_count; i++) {
			g_input_stream_read(in, &p, sizeof(gpointer), NULL, NULL);
			title = main_ui_read_string(in);
			g_input_stream_read(in, &allocation, sizeof(GtkAllocation), NULL,
			NULL);
			MyOperater *operater = main_ui_add(ui, ui);
			opdata = operater_get_opdata(operater);
			my_operater_set_title(operater, title);
			g_free(title);
			gtk_layout_move(my_mainui_get_layout(ui), operater, allocation.x,
					allocation.y);
			tem_list = main_ui_read_list(tem_list, in);
			tem_list = g_list_first(tem_list);
			while (tem_list != NULL) {
				task_setting = g_hash_table_lookup(task_table, tem_list->data);
				task = operater_add_task_with_set(operater, opdata,
						task_setting);
				g_hash_table_insert(task_id_table, tem_list->data, task);
				if (tem_list->next == NULL) {
					g_list_free(tem_list);
					tem_list = NULL;
				} else {
					tem_list = tem_list->next;
				}
			}
		}
		//读取连接；
		g_input_stream_read(in, &link_count, sizeof(guint), NULL, NULL);
		for (i = 0; i < link_count; i++) {
			g_input_stream_read(in, &p, sizeof(gpointer), NULL, NULL);
			tem_list = main_ui_read_list(NULL, in);
			g_list_first(tem_list);
			tem_list2 = NULL;
			while (tem_list != NULL) {
				tem_list2 = g_list_append(tem_list2,
						g_hash_table_lookup(task_id_table, tem_list->data));
				if (tem_list->next == NULL) {
					g_list_free(tem_list);
					tem_list = NULL;
				} else {
					tem_list = tem_list->next;
				};
			}
			g_hash_table_insert(task_to_linklist,
					g_hash_table_lookup(task_id_table, p),
					g_list_first(tem_list2));
		}
		//读取外部cookies文件
		if (COOKIE_FILE != NULL)
			g_free(COOKIE_FILE);
		COOKIE_FILE = main_ui_read_string(in);
		soup_session_add_feature(session,
				soup_cookie_jar_db_new(COOKIE_FILE, TRUE));
		//读取附加请求头值
		main_ui_load_additional_header_request(in);

		g_print("Open file:%s\n", filename);
		g_free(filename);
		g_io_stream_close(io, NULL, NULL);
		g_object_unref(file);
		g_hash_table_unref(task_id_table);
		g_hash_table_unref(task_table);
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);
}

void main_ui_setting(MyMainui *ui, gpointer userdata) {

	MyMainuiSetting *set = my_mainui_setting_new(COOKIE_FILE, MAX_THREAD,
			additional_head_request);
	gtk_window_set_transient_for(set, ui);
	gint i = gtk_dialog_run(set);
	switch (i) {
	case GTK_RESPONSE_OK:
		if (COOKIE_FILE != NULL)
			g_free(COOKIE_FILE);
		COOKIE_FILE = my_mainui_setting_get_cookie_filename(set);
		soup_session_remove_feature_by_type(session, SOUP_TYPE_COOKIE_JAR_DB);
		MAX_THREAD = my_mainui_setting_get_thread_num(set);
		if (COOKIE_FILE != NULL)
			soup_session_add_feature(session,
					soup_cookie_jar_db_new(COOKIE_FILE, TRUE));
		break;
	default:
		break;
	}
	gtk_widget_destroy(set);
}
;

void main_ui_down_info(MyMainui *ui, gpointer userdata) {
	gtk_window_present(down_win);
}

void clear_log_task_href(gpointer key, gpointer value, gpointer user_data) {
	GString *str = value;
	g_string_free(str, TRUE);
}
;

void main_ui_stop(MyMainui *ui, gpointer userdata) {
	MyTaskMessage *task_msg = NULL;
	stop_thread = TRUE;
	g_mutex_lock(log_task_href_mutex);
	if (log_task_href != NULL) {
		g_hash_table_foreach(log_task_href, clear_log_task_href, NULL);
		g_hash_table_unref(log_task_href);
		log_task_href = NULL;
	}
	g_mutex_unlock(log_task_href_mutex);
	task_msg = g_async_queue_try_pop(process_queue);
	while (task_msg != NULL) {
		my_task_message_free(task_msg);
		task_msg = g_async_queue_try_pop(process_queue);
	}
}

void main_log(MyTaskMessage *msg, gchar *format, ...) {
	static gchar buf[4096];
	gint i = 0;
	op_data *data = task_get_opdata(msg->task);
	gchar *log_filename = g_strdup_printf("%s/log.txt", OUTPUT_DIR);
	va_list va;
	va_start(va, format);
	g_mutex_lock(log_mutex);
	i = g_vsnprintf(buf, 4096, format, va);
	if (g_access(log_filename, F_OK | W_OK) != 0) {
		g_io_channel_unref(log_file);
		log_file = g_io_channel_new_file(log_filename, "w+", NULL);
	};
	if (i > 0) {
		gchar *op_title = my_operater_get_title(data->operater);
		gchar *str = g_strdup_printf("Op%s:%d--%s\n\n", op_title, msg->id, buf);
		g_io_channel_write_chars(log_file, str, -1, NULL, NULL);
		g_io_channel_flush(log_file, NULL);
		g_free(str);
	}
	g_free(log_filename);
	g_mutex_unlock(log_mutex);
	va_end(va);
}

char* task_thread_output_file_setname(task_set *set, MyTaskMessage *task_msg) {
	gint i = 0;
	GHashTable *head_table;
	GFile *file;
	gchar *name = NULL, **urlv, *abs_name, *name_temp, *time = NULL, *date =
	NULL, *title =
	NULL;
	gchar *uri = NULL, *uri_temp, *dis_type, *content_type;
	GDateTime *datetime_now;
	//%f参数和%u参数
	if (g_strstr_len(set->fmt_filename, -1, "%f") != NULL
			|| g_strstr_len(set->fmt_filename, -1, "%u") != NULL) {
		uri_temp = soup_uri_to_string(task_msg->uri, FALSE);
		urlv = g_strsplit(uri_temp, "/", -1);
		uri = g_strjoinv("|", urlv);

		name = g_strdup(task_msg->suggest_filename);
		if (name == NULL) { //从uri中获取文件名
			file = g_file_new_for_uri(uri_temp);
			name = g_file_get_basename(file);
			if (g_strcmp0("", name) == 0
					|| g_strcmp0(G_DIR_SEPARATOR_S, name) == 0) {
				g_free(name);
				name = NULL;
			}
			g_object_unref(file);
		}

		if (name == NULL) { //以上方式文件名无效，以uri作为文件名
			name = g_strdup(uri);
		}
		g_free(uri_temp);
		g_strfreev(urlv);
	}
	//%d和%t参数
	if (g_strstr_len(set->fmt_filename, -1, "%d") != NULL
			|| g_strstr_len(set->fmt_filename, -1, "%t") != NULL) {
		datetime_now = g_date_time_new_now_local();
		date = g_date_time_format(datetime_now, "%Y-%m-%d");
		time = g_date_time_format(datetime_now, "%H:%M:%S");

	}
	//%w参数
	if (g_strstr_len(set->fmt_filename, -1, "%w") != NULL) {
		if (task_msg->web_title == NULL) {
			title = g_strdup("");
		} else {
			title = g_strdup(task_msg->web_title);
		}
	};
	name_temp = g_strdup(set->fmt_filename);
	if (date != NULL) {
		uri_temp = name_temp;
		name_temp = g_regex_replace(fmt_filename_date, set->fmt_filename, -1, 0,
				date, 0, NULL);
		g_free(date);
	}
	if (time != NULL) {
		uri_temp = name_temp;
		name_temp = g_regex_replace(fmt_filename_time, uri_temp, -1, 0, time, 0,
		NULL);
		g_free(time);
		g_free(uri_temp);
	}
	if (title != NULL) {
		uri_temp = name_temp;
		name_temp = g_regex_replace(fmt_filename_title, uri_temp, -1, 0, title,
				0, NULL);
		g_free(title);
		g_free(uri_temp);
	}
	if (uri != NULL) {
		uri_temp = name_temp;
		name_temp = g_regex_replace(fmt_filename_uri, uri_temp, -1, 0, uri, 0,
		NULL);
		g_free(uri);
		g_free(uri_temp);
	}
	if (name != NULL) {
		uri_temp = name_temp;
		name_temp = g_regex_replace(fmt_filename_org, uri_temp, -1, 0, name, 0,
		NULL);
		g_free(name);
		g_free(uri_temp);
	}
	if (set->search_xpath) {
		abs_name = g_strdup_printf("xpath-%s", name_temp);
	} else {
		abs_name = g_strdup(name_temp);
	}
	g_free(name_temp);
#ifdef G_OS_WIN32
	urlv = g_strsplit_set(abs_name, "/|\"*?:<>", -1);
	g_free(abs_name);
	abs_name = g_strjoinv("_", urlv);
	g_strfreev(urlv);
#endif
	return abs_name;
}
/*
 gchar* task_curl_set_filename_callback(gchar *suggest_name,
 MyTaskMessage *task_msg) {
 gchar *name, *temp;
 if (task_msg == NULL)
 return NULL;
 task_set *set = task_get_set(task_msg->task);
 g_free(task_msg->suggest_filename);
 task_msg->suggest_filename = g_strdup(suggest_name);
 name = task_thread_output_file_setname(set, task_msg);
 return name;
 }
 ;

 void task_curl_set_cookies_callback(gchar *cookies, MyTaskMessage *task_msg) {
 if (task_msg == NULL)
 return;
 SoupCookieJar *cookiejar = soup_session_get_feature(task_msg->session,
 SOUP_TYPE_COOKIE_JAR);
 soup_cookie_jar_set_cookie(cookiejar, task_msg->uri, cookies);
 }
 ;

 gchar * task_my_curl_get_cookies_callback(MyTaskMessage *task_msg) {
 gchar *cookies = NULL;
 GString *str = g_string_new("");
 SoupCookieJar *cookiejar = soup_session_get_feature(task_msg->session,
 SOUP_TYPE_COOKIE_JAR);
 if (cookiejar != NULL) {
 cookies = soup_cookie_jar_get_cookies(cookiejar, task_msg->uri, FALSE);
 g_string_append_printf(str, "%s;", cookies);
 g_free(cookies);
 cookies = str->str;
 g_string_free(str, FALSE);
 }
 return cookies;
 }
 ;

 gchar ** task_my_curl_get_proxy_callback(gchar *uri, MyTaskMessage *task_msg) {
 GProxyResolver *resolver = NULL;
 SoupSession *s = session;
 if (task_msg != NULL)
 s = task_msg->session;
 g_object_get(s, "proxy-resolver", &resolver, NULL);
 if (resolver == NULL)
 return NULL;
 return g_proxy_resolver_lookup(resolver, uri, NULL, NULL);
 }
 ;

 void task_my_curl_finish_callback(MyTaskMessage *task_msg) {
 task_set *set = task_get_set(task_msg->task);
 if (set->link_wait)
 runing_count_decrease();
 }
 */

gchar* task_soup_dl_set_name(MySoupDl *dl, const gchar *uri,
		const gchar *suggest_name, MyTaskMessage *task_msg, gpointer data) {
	gchar *name;
	if (task_msg == NULL)
		return NULL;
	task_set *set = task_get_set(task_msg->task);
	g_free(task_msg->suggest_filename);
	task_msg->suggest_filename = g_strdup(suggest_name);
	name = task_thread_output_file_setname(set, task_msg);
	return name;
}
;

void task_soup_dl_download_finish(MySoupDl *dl, const gchar *uri,
		const gchar *filename, const gchar *local, MyTaskMessage **task_msg) {
	if (*task_msg != NULL) {
		MyTaskMessage *t = *task_msg;
		task_set *set = task_get_set(t->task);
		if (set->link_wait)
			runing_count_decrease();
		my_task_message_free(t);
		*task_msg = NULL;
	}
}
;

void task_thread_output_file(task_set *set, MyTaskMessage *task_msg,
		gchar *xpath_result) {
	GError *error = NULL;
	gchar *output_dir = my_download_ui_get_default_dir(down_ui);
	gchar *name = g_strdup_printf("%s%s%s", output_dir, G_DIR_SEPARATOR_S,
			task_msg->filename);
#ifdef G_OS_WIN32
	gchar *temp=g_convert(name,-1,"GB2312","UTF-8",NULL,NULL,NULL);
	g_free(name);
	name=temp;
#endif
	GFile *file = g_file_new_for_path(name);
	GFile *parent = g_file_get_parent(file);
	gchar *path = g_file_get_path(parent);
	GIOChannel *io;
	if (g_access(path, F_OK) != 0)
		g_mkdir_with_parents(path, 0775);
	if (set->search_xpath) {
		if (g_strcmp0("", xpath_result) != 0)
			if (g_access(name, 00) == 0) {
				io = g_io_channel_new_file(name, "a", NULL);
				g_io_channel_write_chars(io, xpath_result, -1, NULL, NULL);
				g_io_channel_shutdown(io, TRUE, NULL);
				g_io_channel_unref(io);
			} else {
				g_file_set_contents(name, xpath_result, -1, &error);
			}
	} else {
		g_file_set_contents(name, task_msg->msg->response_body->data,
				task_msg->msg->response_body->length, &error);
	}
	if (error != NULL) {
		main_log(task_msg, "\tWrite File Error:%s\n", error->message);
		g_printerr("Write File Error:%s\n", error->message);
		g_error_free(error);
	}
	g_free(path);
	g_object_unref(parent);
	g_object_unref(file);
	g_free(name);
	g_free(output_dir);
}
;

void task_thread_terminal_print(task_set *set, MyTaskMessage *task_msg,
		gchar *xpath_result) {
	gchar *temp = soup_uri_to_string(task_msg->uri, FALSE);
	if (set->terminal_print) {
		if (set->search_xpath) {
			g_print(
					"#####URL:\"%s\"\n#####Xpath:\"%s\"\n#####Search result:\n%s\n",
					temp, set->xpath, xpath_result);
		} else {
			g_print("#####URL:\"%s\"\n#####Response:\n", temp);
			printf("%s\n", task_msg->msg->response_body->data);
		}
	};
	g_free(temp);
}
;

void task_load_html_doc(MyTaskMessage *task_msg, task_set *set) {
	GError *error = NULL;
	gchar *url = soup_uri_to_string(task_msg->uri, FALSE);
	xmlDoc *doc = htmlReadMemory(task_msg->msg->response_body->data,
			task_msg->msg->response_body->length, url, "utf-8",
			HTML_PARSE_RECOVER | HTML_PARSE_NOERROR);
	xmlXPathContext *ctxt = xmlXPathNewContext(doc);
	task_msg->doc = doc;
	task_msg->ctxt = ctxt;
	g_free(url);
}

void task_parse_head(SoupMessage *msg, MyTaskMessage *task_msg) {
	gchar *mime_type;
	xmlXPathObjectPtr xpobj;
	GHashTable *type_table;
	task_set *set = task_get_set(task_msg->task);
	gchar *temp1, *temp2, *dis_type, *match_res;
	GHashTable *head_table;
	guint i = 0;
	FILE *f;
	GMatchInfo *match_info = NULL;
	GError *err = NULL;
	g_mutex_lock(&task_msg->mutex);
	mime_type = soup_message_headers_get_content_type(msg->response_headers,
			&type_table);
	if (mime_type != NULL) {
		if (g_strstr_len(mime_type, -1, "text") != NULL) {
			g_regex_match(meta_charset, task_msg->msg->response_body->data, 0,
					&match_info);
			if (g_match_info_matches(match_info)) {
				match_res = g_match_info_fetch(match_info, 0);
				temp1 = g_strstr_len(match_res, -1, "charset=");
				temp1 += 8;
				temp2 = temp1;
				while (*temp2 != ' ' && *temp2 != '\"' && *temp2 != ';') {
					temp2++;
				}
				task_msg->charset = g_utf8_strup(temp1, temp2 - temp1);
				g_free(match_res);
			}
			g_match_info_free(match_info);
			task_load_html_doc(task_msg, set);
			if (!set->title_insist) {
				xpobj = xmlXPathEvalExpression("//title/text()",
						task_msg->ctxt);
				if (xpobj != NULL) {
					switch (xpobj->type) {
					case XPATH_NODESET:
						g_free(task_msg->web_title);
						if (xpobj->nodesetval == NULL
								|| xpobj->nodesetval->nodeNr == 0) {
							task_msg->web_title = g_strdup("title");
							break;
						}
						temp1 = xmlNodeGetContent(
								xpobj->nodesetval->nodeTab[0]);
						i = xmlStrlen(temp1);
						if (task_msg->charset != NULL) {
							task_msg->web_title = g_convert_with_fallback(temp1,
									i, "UTF-8", task_msg->charset, "_", NULL,
									NULL, &err);
							if (err != NULL) {
								g_printerr("%s\n", err->message);
								g_error_free(err);
							}
						} else {
							task_msg->web_title = g_strdup(temp1);
						}
						xmlFree(temp1);
						break;
					default:
						task_msg->web_title = g_strdup("title");
					}
					xmlXPathFreeObject(xpobj);
				}
			}

		}
		g_hash_table_unref(type_table);
	}

	if (soup_message_headers_get_content_disposition(msg->response_headers,
			&dis_type, &head_table)) {
		if (g_strcmp0(dis_type, "attachment") == 0) { //从文件头中获取推荐文件名
			task_msg->suggest_filename = g_strdup(
					g_hash_table_lookup(head_table, "filename"));
		}
		g_hash_table_unref(head_table);
	}
	g_mutex_unlock(&task_msg->mutex);
}

gboolean task_check_if_same_uri(MyTaskMessage *task_msg) {
	gchar *temp;
	gboolean result = FALSE;
	g_mutex_lock(log_task_href_mutex);
	GString *log_href = g_hash_table_lookup(log_task_href, task_msg->task);
	temp = soup_uri_to_string(task_msg->uri, FALSE);
	if (log_href == NULL) {
		log_href = g_string_new(temp);
	} else {
		if (g_strrstr(log_href->str, temp) != NULL) {
			result = TRUE;
		} else {
			log_href = g_string_append(log_href, temp);
		}
	};
	g_hash_table_insert(log_task_href, task_msg->task, log_href);
	g_mutex_unlock(log_task_href_mutex);
	g_free(temp);
	return result;
}

xmlXPathObject* task_search_xpath(task_set *set, MyTaskMessage *task_msg) {
	if (set->search_xpath != TRUE)
		return NULL;
	xmlXPathObject *xobj = xmlXPathEvalExpression(set->xpath, task_msg->ctxt);
	return xobj;
}
;

GArray* task_regex_match(const gchar *regex_pattern,
		const gchar *eval_content) {
	GRegex *regex;
	GMatchInfo *info;
	gchar *match;
	GArray *res = g_array_new(TRUE, FALSE, sizeof(gpointer));
	if (g_strcmp0(regex_pattern, "") == 0)
		regex = g_regex_new(".+", 0, 0, NULL);
	else
		regex = g_regex_new(regex_pattern, 0, 0, NULL);
	if (regex == NULL)
		return NULL;
	g_regex_match(regex, eval_content, 0, &info);
	while (g_match_info_matches(info)) {
		match = g_match_info_fetch(info, 0);
		g_print("match:%s\n", match);
		if (match != NULL)
			g_array_append_val(res, match);
		g_match_info_next(info, NULL);
	}
	g_match_info_free(info);
	g_regex_unref(regex);
	return res;
}

GArray* task_xpath_content_fmt(const MyTaskMessage *task_msg, const gchar *fmt,
		const GArray *content) {
	guint i = 0;
	gchar *uri, *title, *text, *t1, *t2;
	gboolean u = FALSE, t = FALSE, f = FALSE;
	GArray *res = g_array_new(TRUE, FALSE, sizeof(gpointer));
	if (task_msg != NULL) {
		uri = soup_uri_to_string(task_msg->uri, FALSE);
		title = g_strdup(task_msg->web_title);
	} else {
		uri = g_strdup("http://www.test.com");
		title = g_strdup("title");
	}
	if (g_strcmp0(fmt, "") == 0) {
		f = TRUE;
		t1 = g_strdup("%f");
	} else {
		if (g_strstr_len(fmt, -1, "%f") != NULL)
			f = TRUE;
		if (g_strstr_len(fmt, -1, "%u") != NULL)
			u = TRUE;
		if (g_strstr_len(fmt, -1, "%w") != NULL)
			t = TRUE;
		t1 = g_strdup(fmt);
	}
	if (u) {
		t2 = t1;
		t1 = g_regex_replace(fmt_filename_uri, t2, -1, 0, uri, 0, NULL);
		g_free(t2);
	}
	if (t) {
		t2 = t1;
		t1 = g_regex_replace(fmt_filename_title, t2, -1, 0, title, 0, NULL);
		g_free(t2);
	}
	while (i < content->len) {
		text = g_array_index(content, gpointer, i);
		if (f) {
			t2 = g_regex_replace(fmt_filename_org, t1, -1, 0, text, 0, NULL);
		} else {
			t2 = g_strdup(t1);
		}
		g_array_append_val(res, t2);
		i++;
	}
	g_free(uri);
	g_free(title);
	g_free(t1);
	return res;
}

void task_xpath_output(MyTaskMessage *task_msg, const gchar *content) {
	MyTaskMessage *sub_task_msg;
	task_set *sub_set, *set;
	SoupURI *uri = soup_uri_new_with_base(task_msg->uri, content);
	GList *task_link = task_get_linklist(task_msg->task);
	while (task_link != NULL) {
		if (uri == NULL)
			break;
		if (uri->host == NULL)
			break;
		if (stop_thread == FALSE) {
			sub_task_msg = my_task_message_new(session, soup_uri_copy(uri),
					task_link->data, task_id++);
			sub_task_msg->web_title = g_strdup(task_msg->web_title);
			sub_set = task_get_set(task_link->data);
			set = task_get_set(task_msg->task);
			if ((sub_set->search_xpath == FALSE && sub_set->output_file)
					|| set->link_wait) {
				runing_count++;
				task_source(sub_task_msg);
			} else
				g_async_queue_push(process_queue, sub_task_msg);
		}
		task_link = task_link->next;
	}
	soup_uri_free(uri);
}

GArray* task_regex_and_fmt(MyTaskMessage *task_msg, guchar *content) {
	GList *task_link = task_get_linklist(task_msg->task);
	GArray *regex_arr, *fmt_arr;
	task_set *set = task_get_set(task_msg->task);
	guint i, j, output = 0;
	xmlChar *temp;
	//正规表达式是否非空
	if (g_strcmp0(set->regex_pattern, "") != 0) {
		//非空则以该正规表达式匹配Xpath结果
		regex_arr = task_regex_match(set->regex_pattern, content);
	} else {
		//空则直接复制Xpath结果
		regex_arr = g_array_new(TRUE, FALSE, sizeof(gpointer));
		temp = g_strdup(content);
		g_array_append_val(regex_arr, temp);
	}
	//Xpath结果格式化是否非空
	if (g_strcmp0(set->fmt_output, "") != 0) {
		//非空则以该格式格式化内容
		fmt_arr = task_xpath_content_fmt(task_msg, set->fmt_output, regex_arr);
	} else {
		//空则直接不处理内容，直接复制
		fmt_arr = g_array_new(TRUE, FALSE, sizeof(gpointer));
		for (j = 0; j < regex_arr->len; j++) {
			temp = g_strdup(g_array_index(regex_arr, gpointer, j));
			g_array_append_val(fmt_arr, temp);
		}
	}
	g_ptr_array_set_free_func(regex_arr, g_free);
	g_ptr_array_set_free_func(fmt_arr, g_free);
	g_array_free(regex_arr, TRUE);
	return fmt_arr;
}

gint task_xpath_eval(MyTaskMessage *task_msg) {
	GArray *a, *b;
	xmlXPathObject *xpobj;
	xmlNodeSet *ns;
	guint i, j, output = 0;
	xmlChar *temp;
	guchar *content;
	GList *task_link = task_get_linklist(task_msg->task);
	gchar *u = soup_uri_to_string(task_msg->uri, FALSE);
	//MyTaskMessage *sub_task_msg;
	GArray *fmt_arr;
	GString *str = task_msg->xpath_result, *output_log = g_string_new("");
	task_set *set = task_get_set(task_msg->task);
	g_string_append_printf(output_log, "Parse Xpath\n\tURI:%s\n\tXPATH:%s\n", u,
			set->xpath);
	//main_log(task_msg, "Parse Xpath\n\tXpath:%s\n", set->xpath);
	xpobj = task_search_xpath(set, task_msg);
	if (xpobj != NULL && xpobj->nodesetval != NULL) {
		ns = xpobj->nodesetval;
		g_string_printf(str, "\n\n####### XPATH: %s\n####### URI: %s\n",
				set->xpath, u);
		if (task_msg->web_title != NULL)
			g_string_append_printf(str, "####### TITLE: %s\n",
					task_msg->web_title);
		if (task_link != NULL) {
			for (i = 0; i < ns->nodeNr; i++) {
				temp = xmlNodeGetContent(ns->nodeTab[i]);
				if (temp == NULL) {
					//Xpath分析内容为空
					continue;
				}
				temp = g_strstrip(temp);
				if (task_msg->charset != NULL) {
					//编码转换
					content = g_convert(temp, -1, "utf-8", task_msg->charset,
					NULL,
					NULL, NULL);
				} else {
					content = g_strdup(temp);
				}
				g_free(temp);
				if (content == NULL) {
					i++;
					continue;
				}

				g_string_append_printf(str, "\t\"%s\"\n", (gchar*) content);
				if (stop_thread == FALSE) {
					//Xpath结果是否要修整
					if (set->output_modify) {
						fmt_arr = task_regex_and_fmt(task_msg, content);
						g_string_append_printf(str, "\t  =>\n");
						for (j = 0; j < fmt_arr->len; j++) {
							task_xpath_output(task_msg,
									g_array_index(fmt_arr, gpointer, j));
							g_string_append_printf(output_log,
									"\t  \"%s\"\n\t  =>:\"%s\"\n", content,
									g_array_index(fmt_arr, gpointer, j));
							g_string_append_printf(str, "\t    \"%s\"\n",
									g_array_index(fmt_arr, gpointer, j));
							output++;
						}
						g_array_free(fmt_arr, TRUE);
					} else {
						//Xpath结果不用修整
						g_string_append_printf(output_log, "\t  \"%s\"\n",
								content);
						task_xpath_output(task_msg, content);
						output++;
					}
				}
				g_free(content);
			};
		} else {
			for (i = 0; i < ns->nodeNr; i++) {
				content = xmlNodeGetContent(ns->nodeTab[i]);
				if (content == NULL) {
					i++;
					continue;
				}
				content = g_strstrip(content);
				g_string_append_printf(str, "\t\"%s\"\n", (gchar*) content);
				g_string_append_printf(output_log, "\t  \"%s\"\n", content);
				g_free(content);
				output++;
			};
		};
	};
	xmlXPathFreeObject(xpobj);
	g_free(u);
	main_log(task_msg, "%s", output_log->str);
	g_string_free(output_log, TRUE);
	return output;
}

gboolean task_process(MyTaskMessage *task_msg) {
	gint output = 0;
	gchar *content;
	gint i;
	gboolean nonnull_break = FALSE;
	task_set *task_setting = task_get_set(task_msg->task);
	if (task_setting->search_xpath) {
		output = task_xpath_eval(task_msg);
	} else if (task_setting->output_modify) {
		if (task_msg->charset != NULL) {
			//编码转换
			content = g_convert(task_msg->msg->response_body->data, -1, "utf-8",
					task_msg->charset,
					NULL,
					NULL, NULL);
		} else {
			content = g_strdup(task_msg->msg->response_body->data);
		}
		GArray *fmt_arr = task_regex_and_fmt(task_msg, content);
		if (fmt_arr != NULL) {
			for (i = 0; i < fmt_arr->len; i++) {
				task_xpath_output(task_msg,
						g_array_index(fmt_arr, gpointer, i));
				main_log(task_msg, "\t  \"%s\"\n",
						g_array_index(fmt_arr, gpointer, i));
				g_string_append_printf(task_msg->xpath_result, "\t    \"%s\"\n",
						g_array_index(fmt_arr, gpointer, i));
				output++;
			}
		}
		g_array_free(fmt_arr, TRUE);
		g_free(content);
	}
	if (task_setting->output_file) {
		g_free(task_msg->filename);
		task_msg->filename = task_thread_output_file_setname(task_setting,
				task_msg);
		main_log(task_msg, "Write to File\n\tFile:%s\n", task_msg->filename);
		task_thread_output_file(task_setting, task_msg,
				task_msg->xpath_result->str);
	}
	if (task_setting->terminal_print)
		task_thread_terminal_print(task_setting, task_msg,
				task_msg->xpath_result->str);
	g_string_free(task_msg->xpath_result, TRUE);
	task_msg->xpath_result = g_string_new("");
	op_data *data = task_get_opdata(task_msg->task);
	GList *task_list = g_list_find(data->task, task_msg->task);
	if (task_setting->nonnull_break && output > 0) {
		//结果非空退出
		nonnull_break = TRUE;
	}
	if (task_list->next == NULL || stop_thread == TRUE
			|| nonnull_break == TRUE) {
		//退出下一级任务的执行。
		my_task_message_free(task_msg);
	} else {
		task_msg->task = task_list->next->data;
		runing_count++;
		task_process(task_msg);
	}
	runing_count_decrease();
	return G_SOURCE_REMOVE;
}

void task_send_message_callback(SoupSession *session, SoupMessage *msg,
		MyTaskMessage *task_msg) {
	gchar *uri;
	if (msg->status_code == SOUP_STATUS_OK) {
		//网页载入成功
		task_parse_head(msg, task_msg);
		g_idle_add(task_process, task_msg);
	} else {
		//网页载入失败
		uri = soup_uri_to_string(task_msg->uri, FALSE);
		if (task_msg->reply < my_download_ui_get_reply(down_ui)
				&& task_msg->msg->status_code != SOUP_STATUS_CANCELLED) {
			//若失败次数少于允许重链接数目，重新连接。
			task_msg->reply++;
			task_msg->msg = g_object_ref(msg);
			main_log(task_msg,
					"Try To Reload Uri \n\tUri:%s\n\tFailed Reson:%s\n\tReply:%d / %d\n",
					uri, soup_status_get_phrase(msg->status_code),
					task_msg->reply, my_download_ui_get_reply(down_ui));
			soup_session_queue_message(task_msg->session, msg,
					task_send_message_callback, task_msg);
		} else {
			//若失败次数大于允许数目，停止继续链接。
			main_log(task_msg, "Load Uri Failed!!!\n\tUri:%s\n\tReson:%s\n",
					uri, soup_status_get_phrase(msg->status_code));
			runing_count_decrease();
			my_task_message_free(task_msg);
		}
		g_free(uri);
	}
}

gboolean task_source(MyTaskMessage *task_msg) {
	gchar *uri, *header_name = NULL, *header_value = NULL;
	SoupMessage *msg, *t;
	GtkTreeIter iter;
	task_set *task_setting = task_get_set(task_msg->task);
	if (task_msg->uri == NULL) {
		my_task_message_free(task_msg);
		runing_count_decrease();
		return G_SOURCE_REMOVE;
	}
	if (task_setting->same_uri_skip
			&& task_check_if_same_uri(task_msg) && task_msg->msg == NULL) {
		//如果载入链接为空或链接重复载入，则停止任务处理
		uri = soup_uri_to_string(task_msg->uri, FALSE);
		main_log(task_msg, "Skip Same Uri\n\tUri:%s\n", uri);
		g_free(uri);
		my_task_message_free(task_msg);
		runing_count_decrease();
		return G_SOURCE_REMOVE;
	}
	if (task_msg->msg == NULL) {
		uri = soup_uri_to_string(task_msg->uri, FALSE);
		if (task_setting->search_xpath == FALSE
				&& task_setting->output_file == TRUE) {
			main_log(task_msg, "DownLoad\n\tUri:%s\n", uri);
			/*my_curl_add_download(mycurl, uri, NULL, NULL, NULL, NULL, NULL,
			 g_object_ref(task_msg), g_object_ref(task_msg),
			 g_object_ref(task_msg), g_object_ref(task_msg), FALSE);*/
			t = g_object_ref(task_msg);
			my_soup_dl_add_download(mysoupdl, uri, t);
			g_object_unref(task_msg);
			if (!task_setting->link_wait)
				runing_count_decrease();

		} else {
			main_log(task_msg, "Load Uri\n\tUri:%s\n", uri);
			msg = soup_message_new_from_uri("GET", task_msg->uri);
			gtk_tree_model_get_iter_first(additional_head_request, &iter);
			do {
				gtk_tree_model_get(additional_head_request, &iter, 0,
						&header_name, 1, &header_value, -1);
				if (g_strcmp0("", header_name) != 0) {
					soup_message_headers_replace(msg->request_headers,
							header_name, header_value);
				}
				g_free(header_name);
				g_free(header_value);
			} while (gtk_tree_model_iter_next(additional_head_request, &iter));

//			soup_message_headers_replace(msg->request_headers,\
//					"User-Agent",\
//					"Mozilla/5.0 (iPhone; CPU iPhone OS 12_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/12.0 Mobile/15A372 Safari/604.1"\
//					);

			task_msg->msg = g_object_ref(msg);
			soup_session_queue_message(task_msg->session, msg,
					task_send_message_callback, task_msg);
		}
		g_free(uri);
		return G_SOURCE_REMOVE;
	}
	return G_SOURCE_REMOVE;
}

void main_ui_exec(MyMainui *ui, gpointer *userdata) {
	op_data *data;
	task_set *set;
	gchar *str;
	GIOChannel *io;
	SoupURI *uri;
	MyTaskMessage *task_msg;
	if (log_task_href != NULL) {
		g_hash_table_foreach(log_task_href, clear_log_task_href, NULL);
		g_hash_table_unref(log_task_href);
		log_task_href = NULL;
	}
	log_task_href = g_hash_table_new(g_direct_hash, g_direct_equal);
	GList *op_list = g_hash_table_get_keys(operater_to_opdata);
	if (op_list == NULL)
		return;
	op_list = g_list_first(op_list);
	while (op_list != NULL) {
		g_print("Task Start\n");
		data = operater_get_opdata(op_list->data);
		if (data->task == NULL)
			continue;
		set = task_get_set(data->task->data);
		switch (set->source) {
		case TASK_SOURCE_FILE:
			if (g_access(set->source_name, 04) != 0)
				break;
			io = g_io_channel_new_file(set->source_name, "r", NULL);
			while (g_io_channel_read_line(io, &str, NULL, NULL,
			NULL) != G_IO_STATUS_EOF) {
				uri = soup_uri_new(str);
				task_msg = my_task_message_new(session, uri, data->task->data,
						task_id++);
				g_async_queue_push(process_queue, task_msg);
			}
			g_io_channel_close(io);
			g_io_channel_unref(io);
			break;
		case TASK_SOURCE_URL:
			task_msg = my_task_message_new(session,
					soup_uri_new(set->source_name), data->task->data,
					task_id++);
			g_async_queue_push(process_queue, task_msg);
			break;
		default:
			break;
		};
		if (op_list->next == NULL) {
			g_list_free(op_list);
			op_list = NULL;
		} else {
			op_list = op_list->next;
		};
	};
	g_print("Exec Finish\n");
	return;
}
;

gboolean layout_draw_link(GtkWidget *widget, cairo_t *cr) {
	GList *tasklist = g_hash_table_get_keys(task_to_linklist);
	GList *taskdest;
	op_data *src_opdata, *dest_opdata;
	GtkAllocation src_alloc, des_alloc, op_alloc, des_op_alloc;
	gint y;
	tasklist = g_list_first(tasklist);
	cairo_save(cr);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width(cr, 8);

	while (tasklist != NULL) {
		taskdest = g_hash_table_lookup(task_to_linklist, tasklist->data);
		taskdest = g_list_first(taskdest);
		gtk_widget_get_allocation(tasklist->data, &src_alloc);
		src_opdata = task_get_opdata(tasklist->data);
		while (taskdest != NULL) {
			if (MY_IS_TASK(taskdest->data)) {
				dest_opdata = task_get_opdata(taskdest->data);

				gtk_widget_get_allocation(taskdest->data, &des_alloc);
				gtk_widget_get_allocation(dest_opdata->operater, &des_op_alloc);
				gtk_widget_get_allocation(src_opdata->operater, &op_alloc);

				if (src_opdata->operater == dest_opdata->operater) {
					//连接目标op同为源op
					cairo_set_source_rgba(cr, 0, 0, 1, 0.5);
					cairo_move_to(cr, src_alloc.x + src_alloc.width,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							op_alloc.y + op_alloc.height + 20);
					cairo_line_to(cr, des_alloc.x - 40,
							op_alloc.y + op_alloc.height + 20);
					cairo_line_to(cr, des_alloc.x - 40,
							des_alloc.y + des_alloc.height / 2);
					cairo_line_to(cr, des_alloc.x,
							des_alloc.y + des_alloc.height / 2);
					cairo_stroke(cr);
				} else if ((op_alloc.x + op_alloc.width) > des_alloc.x) {
					//连接目标op在源op的左边
					cairo_set_source_rgba(cr, 0, 1, 0, 0.5);
					cairo_move_to(cr, src_alloc.x + src_alloc.width,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							src_alloc.y + src_alloc.height / 2);
					if (des_op_alloc.y > op_alloc.y
							&& des_op_alloc.y
									< (op_alloc.y + op_alloc.height)) {
						//目标op头在源op内
						y = des_op_alloc.y + des_op_alloc.height + 20;
					} else if ((des_op_alloc.y + des_op_alloc.height)
							> op_alloc.y
							&& (des_op_alloc.y + des_op_alloc.height)
									< (op_alloc.y + op_alloc.height)) {
						//目标op尾在源op内
						y = op_alloc.y + op_alloc.height + 20;
					} else if (des_op_alloc.y > op_alloc.y) {
						//目标op头部留有空间
						y = (op_alloc.y + op_alloc.height + des_op_alloc.y) / 2;
					} else {
						//目标op尾部留有空间
						y = (des_op_alloc.y + des_op_alloc.height + op_alloc.y)
								/ 2;
					}
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40, y);
					cairo_line_to(cr, des_alloc.x - 40, y);
					cairo_line_to(cr, des_alloc.x - 40,
							des_alloc.y + des_alloc.height / 2);
					cairo_line_to(cr, des_alloc.x,
							des_alloc.y + des_alloc.height / 2);
					cairo_stroke(cr);
				} else {
					cairo_set_source_rgba(cr, 0, 1, 0, 0.5);

					cairo_move_to(cr, src_alloc.x + src_alloc.width,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, des_alloc.x - 40,
							des_alloc.y + des_alloc.height / 2);
					cairo_line_to(cr, des_alloc.x,
							des_alloc.y + des_alloc.height / 2);
					cairo_stroke(cr);
				}
			} else {
				taskdest = g_list_remove(g_list_first(taskdest),
						taskdest->data);
				g_hash_table_insert(task_to_linklist, tasklist->data, taskdest);
			}
			if (taskdest != NULL)
				taskdest = taskdest->next;
		}
		if (tasklist->next == NULL) {
			g_list_free(tasklist);
			tasklist = NULL;
		} else {
			tasklist = tasklist->next;
		}
	}
	cairo_restore(cr);
	return G_SOURCE_CONTINUE;
}
;

#endif /* MAINFUN_C_ */
