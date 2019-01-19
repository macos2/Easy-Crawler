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
#include <libsoup-2.4/libsoup/soup.h>
#include <libxml2/libxml/xpath.h>
#include <libxml2/libxml/HTMLparser.h>
#include "mainui.h"
#include "operater.h"
#include "task.h"
#include "task_setting.h"
#include "task_message.h"
#include "mainui_setting.h"
#include "MyCurl.h"

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
GThreadPool *pool;
gchar *OUTPUT_DIR;
gchar *COOKIE_FILE;
GIOChannel *log_file;
GMutex *log_mutex, *log_task_href_mutex;
GMutex notify_download_mutex;
gboolean stop_thread;
MyDownloadUi *down_ui;
GtkWindow *down_win;
GRegex *fmt_filename_date, *fmt_filename_time, *fmt_filename_title,
		*fmt_filename_org, *fmt_filename_uri;
GList *notify_download_list, *finish_download_list;
static gint task_id = 0;
static gint operater_id = 0;
static gchar *size_unit[] = { "Byte", "KiB", "MiB", "GiB", "TiB" };

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

op_data *operater_get_opdata(MyOperater *op) {
	return g_hash_table_lookup(operater_to_opdata, op);
}
;
op_data *task_get_opdata(MyTask *task) {
	return g_hash_table_lookup(task_to_opdata, task);
}

task_set *task_get_set(MyTask *task) {
	op_data *data = g_hash_table_lookup(task_to_opdata, task);
	return g_hash_table_lookup(data->task_set, task);
}

GList *task_get_linklist(MyTask *task) {
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
	g_free(set->output_xpathprop);
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
	if (set->output_xpath) {
		gtk_image_set_from_icon_name(my_task_get_next_icon(self),
				"go-next-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		g_signal_connect(my_task_get_next_event(self), "drag-data-get",
				task_drag_data_get, self);
	} else {
		gtk_image_set_from_icon_name(my_task_get_next_icon(self), "",
				GTK_ICON_SIZE_LARGE_TOOLBAR);
		g_signal_handlers_disconnect_by_func(my_task_get_next_event(self),
				task_drag_data_get, self);
		g_hash_table_remove(task_to_linklist, self);
	};
	switch (set->source) {
	case TASK_SOURCE_FILE:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"text-x-generic-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	case TASK_SOURCE_URL:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"network-transmit-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	default:
		gtk_image_set_from_icon_name(my_task_get_state_icon(self),
				"go-next-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	}
	if (set->search_xpath == TRUE) {
		gtk_image_set_from_icon_name(my_task_get_icon(self),
				"system-search-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		str = g_strdup_printf("%s/[@%s]", set->xpath, set->output_xpathprop);
		gtk_label_set_text(my_task_get_label(self), str);
		g_free(str);
	} else {
		if (set->output_file == TRUE) {
			gtk_image_set_from_icon_name(my_task_get_icon(self),
					"folder-download-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
			str = g_strdup_printf("DownLoad \"%s\"", set->fmt_filename);
			gtk_label_set_text(my_task_get_label(self), str);
			g_free(str);
		}
	}
}
;

void task_content_clicked(MyTask *self, gpointer userdata) {
	op_data *data = userdata;
	task_set *set = g_hash_table_lookup(data->task_set, self);
	MyTaskSetting *setting = my_task_setting_new(set);
	gtk_window_set_transient_for(setting, data->ui);
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

MyTask * operater_add_task(MyOperater *self, gpointer userdata) {
	op_data *data = userdata;
	MyTask *task = my_task_new();
	task_set *set = g_malloc(sizeof(task_set));
	my_operater_add(self, task);
	data->task = g_list_append(data->task, task);
	g_hash_table_insert(data->task_set, task, set);
	set->source_name = g_strdup("");
	set->xpath = g_strdup("//a");
	set->output_file = FALSE;
	set->search_xpath = TRUE;
	set->terminal_print = FALSE;
	set->output_xpath = TRUE;
	set->output_xpathprop = g_strdup("href");
	set->fmt_filename = g_strdup("%f");
	set->source = TASK_SOURCE_LINKER;
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

MyTask * operater_add_task_with_set(MyOperater *self, op_data *data,
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

MyOperater * main_ui_add(MyMainui *ui, gpointer userdata) {
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

gchar * main_ui_read_string(GInputStream *in) {
	size_t i;
	g_input_stream_read(in, &i, sizeof(size_t), NULL, NULL);
	if (i == 0)
		return g_strdup("");
	gchar *str = g_malloc0(i);
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

GList * main_ui_read_list(GList *list, GInputStream *in) {
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
	g_input_stream_read(in, &(set->output_xpath), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->search_xpath), sizeof(gboolean), NULL,
	NULL);
	g_input_stream_read(in, &(set->terminal_print), sizeof(gboolean), NULL,
	NULL);
	set->fmt_filename = main_ui_read_string(in);
	set->output_xpathprop = main_ui_read_string(in);
	set->source_name = main_ui_read_string(in);
	set->xpath = main_ui_read_string(in);
	g_input_stream_read(in, &(set->source), sizeof(guint), NULL, NULL);
}
;

void task_set_save_to_file(task_set *set, GOutputStream *out) {
	g_output_stream_write(out, &(set->output_file), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->output_xpath), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->search_xpath), sizeof(gboolean), NULL,
	NULL);
	g_output_stream_write(out, &(set->terminal_print), sizeof(gboolean), NULL,
	NULL);

	main_ui_save_string(set->fmt_filename, out);
	main_ui_save_string(set->output_xpathprop, out);
	main_ui_save_string(set->source_name, out);
	main_ui_save_string(set->xpath, out);
	g_output_stream_write(out, &(set->source), sizeof(guint), NULL, NULL);
}
;

void update_task_address(gpointer data, GHashTable *task_id_table) {
	data = g_hash_table_lookup(task_id_table, data);
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
			task_setting = g_malloc(sizeof(task_set));
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
	MyMainuiSetting *set = my_mainui_setting_new(COOKIE_FILE,
			g_thread_pool_get_max_threads(pool));
	gtk_window_set_transient_for(set, ui);
	gint i = gtk_dialog_run(set);
	switch (i) {
	case GTK_RESPONSE_OK:
		if (COOKIE_FILE != NULL)
			g_free(COOKIE_FILE);
		COOKIE_FILE = my_mainui_setting_get_cookie_filename(set);
		soup_session_remove_feature_by_type(session, SOUP_TYPE_COOKIE_JAR_DB);
		g_thread_pool_set_max_threads(pool,
				my_mainui_setting_get_thread_num(set), NULL);
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
	gtk_widget_show_all(down_win);
}

void clear_log_task_href(gpointer key, gpointer value, gpointer user_data) {
	GString *str = value;
	g_string_free(str, TRUE);
}
;

void main_ui_stop(MyMainui *ui, gpointer userdata) {
	stop_thread = TRUE;
	g_mutex_lock(log_task_href_mutex);
	if (log_task_href != NULL) {
		g_hash_table_foreach(log_task_href, clear_log_task_href, NULL);
		g_hash_table_unref(log_task_href);
		log_task_href = NULL;
	}
	g_mutex_unlock(log_task_href_mutex);
}

void main_log(MyOperater *op, gchar *log) {
	g_mutex_lock(log_mutex);
	gchar *title = my_operater_get_title(op);
	gchar *str = g_strdup_printf("Operater:%s--%s", title, log);
	g_io_channel_write_chars(log_file, str, -1, NULL, NULL);
	g_io_channel_flush(log_file, NULL);
	g_free(str);
	g_mutex_unlock(log_mutex);
}

xmlXPathObject *task_thread_search_xpath(task_set *set,
		MyTaskMessage *task_msg) {
	SoupMessage *msg = task_msg->msg;
	if (set->search_xpath != TRUE)
		return NULL;
	xmlXPathObject *xobj = xmlXPathEvalExpression(set->xpath, task_msg->ctxt);
	return xobj;
}
;

char *task_thread_output_file_setname(task_set *set, MyTaskMessage *task_msg) {
	gint i = 0;
	GHashTable *head_table;
	SoupMessageHeaders *msg_head;
	GFile *file;
	gchar *name = NULL, **urlv, *abs_name, *name_temp, *time = NULL, *date =
	NULL, *title =
	NULL;
	gchar *uri = NULL, *uri_temp, *dis_type, *content_type;
	GDateTime *datetime_now;
	GHashTable *type_table;
	//%f参数和%u参数
	if (g_strstr_len(set->fmt_filename, -1, "%f") != NULL
			|| g_strstr_len(set->fmt_filename, -1, "%u") != NULL) {
		msg_head = task_msg->msg->response_headers;
		if (soup_message_headers_get_content_disposition(msg_head, &dis_type,
				&head_table)) {
			if (g_strcmp0(dis_type, "attachment") == 0) { //从文件头中获取推荐文件名
				name = g_strdup(g_hash_table_lookup(head_table, "filename"));
			}
			g_hash_table_unref(head_table);
		}
		uri_temp = soup_uri_to_string(task_msg->uri, FALSE);
		urlv = g_strsplit(uri_temp, G_DIR_SEPARATOR_S, -1);
		uri = g_strjoinv("|", urlv);

		if (name == NULL) { //从uri中获取文件名
			file = g_file_new_for_path(uri_temp);
			name = g_file_get_basename(file);
			if (g_strcmp0("", name) == 0 || g_strcmp0("/", name) == 0) {
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
	task_msg->filename = abs_name;
	return abs_name;
}

void task_thread_output_file(task_set *set, MyTaskMessage *task_msg,
		gchar *xpath_result) {
	gchar *output_dir = my_download_ui_get_default_dir(down_ui);
	gchar *name = g_strdup_printf("%s%s%s", task_msg->local, G_DIR_SEPARATOR_S,
			task_msg->filename);
	SoupMessage *msg = task_msg->msg;
	GIOChannel *io;
	if (set->search_xpath) {
		if (g_strcmp0("", xpath_result) != 0)
			if (g_access(name, 00) == 0) {
				io = g_io_channel_new_file(name, "a", NULL);
				g_io_channel_write_chars(io, xpath_result, -1, NULL, NULL);
				g_io_channel_shutdown(io, TRUE, NULL);
				g_io_channel_unref(io);
			} else {
				g_file_set_contents(name, xpath_result, -1, NULL);
			}
	} else {
		g_file_set_contents(name, msg->response_body->data,
				msg->response_body->length, NULL);
	}
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

void task_thread_download_got_head(SoupMessage *msg, MyTaskMessage *task_msg) {
	gchar *mime_type;
	xmlXPathObjectPtr xpobj;
	GHashTable *type_table;
	task_set *set = task_get_set(task_msg->task);
	gchar *local = task_msg->local;
	gchar *temp1, *temp2;
	guint i = 0;
	GDateTime *time = g_date_time_new_now_local();
	FILE *f;
	g_mutex_lock(&task_msg->mutex);
	task_msg->GET_STATUS = Get_Head;
	mime_type = soup_message_headers_get_content_type(
			task_msg->msg->response_headers, &type_table);
	if (mime_type != NULL) {
		if (g_strstr_len(mime_type, -1, "text") != NULL) {
			task_msg->charset = g_strdup(
					g_hash_table_lookup(type_table, "charset"));
			task_thread_load_html_doc(task_msg, set);
			xpobj = xmlXPathEvalExpression("//title/text()", task_msg->ctxt);
			if (xpobj != NULL) {
				switch (xpobj->type) {
				case XPATH_STRING:
					g_free(task_msg->web_title);
					task_msg->web_title = g_strdup(xpobj->stringval);
					break;
				case XPATH_NODESET:
					if (xpobj->nodesetval == NULL)
						break;
					g_free(task_msg->web_title);
					task_msg->web_title = xmlNodeGetContent(
							xpobj->nodesetval->nodeTab[0]);
					break;
				default:
					task_msg->web_title = g_strdup("");
				}
				xmlXPathFreeObject(xpobj);
			}
		}
		g_hash_table_unref(type_table);
	}
	g_free(mime_type);
	task_msg->content_size = soup_message_headers_get_content_length(
			msg->response_headers);
	task_msg->filename = task_thread_output_file_setname(set, task_msg);
	temp1 = g_strdup_printf("%s%s%s", local, G_DIR_SEPARATOR_S,
			task_msg->filename);
	if (g_access(temp1, F_OK) == 0) {
		same_name_operation op = my_download_ui_get_same_name_op(down_ui);
		switch (op) {
		case add_suffix:
			temp2 = NULL;
			i=0;
			while (g_access(temp1, F_OK) == 0) {
				g_free(temp1);
				g_free(temp2);
				temp2 = g_strdup_printf("%s.%d", task_msg->filename, i++);
				temp1 = g_strdup_printf("%s%s%s", local, G_DIR_SEPARATOR_S,
						temp2);
			}
			f=fopen(temp1,"w+");
			fwrite("a",sizeof(char),1,f);
			fclose(f);
			g_free(task_msg->filename);
			task_msg->filename = g_strdup(temp2);
			g_free(temp2);
			break;
		case skip_download:
			task_msg->cancel = TRUE;
			break;
		case over_write:
		default:
			break;
		}
	}
	g_free(temp1);
	task_msg->start_time = g_date_time_to_unix(time);
	if (task_msg->cancel) {
		soup_session_cancel_message(task_msg->session, task_msg->msg,
				SOUP_STATUS_CANCELLED);
		task_msg->GET_STATUS = Get_Cancel;
	}
	g_date_time_unref(time);
	g_mutex_unlock(&task_msg->mutex);
}

void task_thread_download_notify(SoupMessage *msg, SoupBuffer *chunk,
		MyTaskMessage *task_msg) {
	if (task_msg->list_row == NULL)
		return; //下载已经取消
	g_mutex_lock(&task_msg->mutex);
	task_msg->dl_size += chunk->length;
	task_msg->GET_STATUS=Get_Body;
	g_mutex_unlock(&task_msg->mutex);
}

void task_thread_download_finish(SoupMessage *msg, MyTaskMessage *task_msg) {
	g_mutex_lock(&task_msg->mutex);
	task_msg->GET_STATUS=Get_Finish;
	g_mutex_unlock(&task_msg->mutex);
}

void task_thread_load_html_doc(MyTaskMessage *task_msg, task_set *set) {
	SoupMessage *msg = task_msg->msg;
	gchar *url = soup_uri_to_string(task_msg->uri, FALSE);
	xmlDoc *doc = htmlReadMemory(msg->response_body->data,
			msg->response_body->length, url, task_msg->charset,
			HTML_PARSE_RECOVER | HTML_PARSE_NOERROR);
	xmlXPathContext *ctxt = xmlXPathNewContext(doc);
	task_msg->doc = doc;
	task_msg->ctxt = ctxt;
	g_free(url);
}

guint task_thread_send_message(MyTaskMessage *task_msg) {
	gchar *time_fmt, *uri_fmt;
	task_set *set = task_get_set(task_msg->task);
	task_msg->msg = soup_message_new_from_uri("GET", task_msg->uri);
	SoupMessage *msg = task_msg->msg;
	task_msg->cancel = FALSE;
	//task_msg->local = my_download_ui_get_default_dir(down_ui);
	if (!set->search_xpath && set->output_file) { //任务只是简单的下载文件任务，设置以下内容；
		task_msg->list_row =NULL;
		task_msg->GET_STATUS = Get_Begin;
		//g_signal_connect(task_msg->msg, "got-chunk",task_thread_download_notify, task_msg);
		//g_signal_connect(task_msg->msg, "finished", task_thread_download_finish,task_msg);
		//g_mutex_lock(&notify_download_mutex);
		//notify_download_list = g_list_append(notify_download_list, task_msg);
		//g_object_ref(task_msg);
		//g_mutex_unlock(&notify_download_mutex);
	}
	g_signal_connect(task_msg->msg, "got-headers",
			task_thread_download_got_head, task_msg);
//获取页面内容
	return soup_session_send_message(task_msg->session, task_msg->msg);
}

void task_thread(MyTaskMessage *task_msg, gpointer userdata) {
	MyTask *task = task_msg->task;
	MyOperater *op = task_get_opdata(task)->operater;
	task_set *set = task_get_set(task);
	xmlXPathObject *xpobj;
	xmlNodeSet *ns;
	GString *str;
	gchar *content, *prop, *result, *temp, *temp2;
	gint i;
	guint status;
	GList *task_list, *task_link;
	MyTaskMessage *sub_task_msg;
	if (stop_thread == TRUE) {
		my_task_message_free(task_msg);
		return;
	}
	g_print("%3d task_begin\n",task_msg->id);
	//检查处理的uri是否有重复。
	g_mutex_lock(log_task_href_mutex);
	GString *log_href = g_hash_table_lookup(log_task_href, task);
	temp = soup_uri_to_string(task_msg->uri, FALSE);
	if (log_href == NULL) {
		log_href = g_string_new("");
	} else {
		if (g_strrstr(log_href->str, temp) != NULL) {
			temp2 = g_strdup_printf("跳过重复链接:%s\n", temp);
			main_log(op, temp2);
			g_free(temp);
			g_free(temp2);
			g_mutex_unlock(log_task_href_mutex);
			my_task_message_free(task_msg);
			return;
		};
	}
	log_href = g_string_append(log_href, temp);
	g_hash_table_insert(log_task_href, task, log_href);
	g_mutex_unlock(log_task_href_mutex);
	g_print("%3d task_send msg\n",task_msg->id);
	//向目标网址发送Get请求
	temp2 = g_strdup_printf("deal with url:%s\n", temp);
	main_log(op, temp2);
	g_free(temp2);
	if (task_msg->msg == NULL) {
		status = task_thread_send_message(task_msg);
		if (status != SOUP_STATUS_OK) {
			g_object_unref(task_msg);
			temp2 = g_strdup_printf("!!错误:%s Response:%s\n", temp,
					soup_status_get_phrase(status));
			main_log(op, temp2);
			g_free(temp2);
			g_free(temp);
			my_task_message_free(task_msg);
			return;
		}
	};
	g_free(temp);
	str = g_string_new("");
	g_print("%3d task_eval xpath \n",task_msg->id);
	//处理Get的response
	if (set->search_xpath) {
		if (task_msg->doc == NULL)
			task_thread_load_html_doc(task_msg, set);
		xpobj = task_thread_search_xpath(set, task_msg);
		if (xpobj != NULL && xpobj->nodesetval != NULL) {
			ns = xpobj->nodesetval;
			if (set->output_xpath
					&& g_strcmp0("", set->output_xpathprop) != 0) {
				g_string_printf(str,
						"\n\n####### XPATH:%s\n####### PROP:%s\n\n", set->xpath,
						set->output_xpathprop);
				for (i = 0; i < ns->nodeNr; i++) {
					content = xmlNodeGetContent(ns->nodeTab[i]);
					if (content == NULL)
						content = g_strdup("null");
					content = g_strstrip(content);
					prop = xmlGetProp(ns->nodeTab[i], set->output_xpathprop);
					if (prop == NULL)
						prop = g_strdup("null");
					prop = g_strstrip(prop);
					if (stop_thread == FALSE) {
						task_link = task_get_linklist(task);
						while (task_link != NULL) {
							sub_task_msg = my_task_message_new(session,
									soup_uri_new_with_base(task_msg->uri, prop),
									task_link->data, task_id++);
							sub_task_msg->web_title = g_strdup(
									task_msg->web_title);
							if (stop_thread == FALSE) {
								g_thread_pool_push(pool, sub_task_msg, NULL);
							} else {
								my_task_message_free(sub_task_msg);
							};
							task_link = task_link->next;
						}
					}
					g_string_append_printf(str, "\"%s\":\"%s\"\n",
							(gchar*) content, (gchar*) prop);
					g_free(prop);
					g_free(content);
				};
			} else {
				g_string_printf(str, "\n\n####### XPATH:%s\n#######\n",
						set->xpath);
				for (i = 0; i < ns->nodeNr; i++) {
					for (i = 0; i < ns->nodeNr; i++) {
						content = xmlNodeGetContent(ns->nodeTab[i]);
						if (content == NULL)
							content = g_strdup("null");
						content = g_strstrip(content);
						g_string_append_printf(str, "\"%s\"\n",
								(gchar*) content);
						g_free(content);
					};
				};
			};
			xmlXPathFreeObject(xpobj);
		};
	};

	g_print("%3d task_output file \n",task_msg->id);
	if (set->output_file) {
		if (task_msg->filename == NULL)
			task_msg->filename = task_thread_output_file_setname(set, task_msg);
		task_thread_output_file(set, task_msg, str->str);
	}
	g_print("%3d task_print termintal \n",task_msg->id);
	if (set->terminal_print)
		task_thread_terminal_print(set, task_msg, str->str);
	g_string_free(str, TRUE);
	op_data *data = task_get_opdata(task);
	task_list = g_list_find(data->task, task);
	if (task_list->next == NULL) {
		my_task_message_free(task_msg);
	} else {
		task_msg->task = task_list->next->data;
		if (stop_thread == FALSE) {
			g_thread_pool_push(pool, task_msg, NULL);
		} else {
			my_task_message_free(task_msg);
		}
	}
	return;
}
;

void main_ui_exec(MyMainui *ui, gpointer *userdata) {
	op_data *data;
	task_set *set;
	gchar *str;
	GIOChannel *io;
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
				task_msg = my_task_message_new(session, soup_uri_new(str),
						data->task->data, task_id++);
				g_thread_pool_push(pool, task_msg, NULL);
			}
			g_io_channel_close(io);
			break;
		case TASK_SOURCE_URL:
			task_msg = my_task_message_new(session,
					soup_uri_new(set->source_name), data->task->data,
					task_id++);
			g_thread_pool_push(pool, task_msg,
			NULL);
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
	GtkAllocation src_alloc, des_alloc, op_alloc;
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
				if (src_opdata->operater == dest_opdata->operater) {
					gtk_widget_get_allocation(src_opdata->operater, &op_alloc);
					cairo_set_source_rgba(cr, 0, 0, 1, 0.5);
					cairo_move_to(cr, src_alloc.x + src_alloc.width,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							src_alloc.y + src_alloc.height / 2);
					cairo_line_to(cr, src_alloc.x + src_alloc.width + 40,
							op_alloc.y + op_alloc.height + 40);
					cairo_line_to(cr, des_alloc.x - 40,
							op_alloc.y + op_alloc.height + 40);
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
