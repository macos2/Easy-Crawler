/*
 * main.c
 *
 *  Created on: 2017年12月19日
 *      Author: tom
 */

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#include "main.h"
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static gint status_bar_id = 0;
gboolean task_finish;
void my_app_init() {
	GDateTime *time = g_date_time_new_now_local();
	gchar *time_str = g_date_time_format(time, "%Y-%m-%d_%H-%M-%S");
	session = soup_session_new();
	operater_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_linklist = g_hash_table_new(g_direct_hash, g_direct_equal);

	log_task_href = NULL;
	OUTPUT_DIR = g_strdup_printf("easy_crawler/%s", time_str);
	g_free(time_str);
	g_mkdir_with_parents(OUTPUT_DIR, 0777);
	pool = g_thread_pool_new(task_thread, NULL, 10,
	FALSE, NULL);
	COOKIE_FILE = NULL;

	down_ui = my_download_ui_new();
	mycurl=my_curl_new(down_ui);
	down_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add(down_win, down_ui);
	gtk_widget_hide(down_win);
	g_signal_connect(down_win, "delete-event", gtk_widget_hide, NULL);
	my_download_ui_set_default_dir(down_ui, OUTPUT_DIR);
	g_date_time_unref(time);

	fmt_filename_date = g_regex_new("%d", 0, 0, NULL);
	fmt_filename_org = g_regex_new("%f", 0, 0, NULL);
	fmt_filename_time = g_regex_new("%t", 0, 0, NULL);
	fmt_filename_title = g_regex_new("%w", 0, 0, NULL);
	fmt_filename_uri = g_regex_new("%u", 0, 0, NULL);

	log_mutex = g_mutex_new();
	g_mutex_init(log_mutex);
	log_task_href_mutex = g_mutex_new();
	g_mutex_init(log_task_href_mutex);
	g_mutex_init(&notify_download_mutex);
	notify_download_list = NULL;
	finish_download_list = NULL;
}

gboolean notify_thread_num(MyMainui *ui) {
	GList *list;
	MyTaskMessage *task_msg;
	GtkStatusbar *status_bar = my_mainui_get_statusbar(ui);
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeRowReference *ref;
	gdouble size, speed;
	gint i, progress;
	gchar *size_fmt, *time_fmt, *ela_time_fmt, *dlsize_size, *uri_fmt,
			*speed_fmt;
	download_state state;
	GDateTime *time = g_date_time_new_now_local();
	time_fmt = g_date_time_format(time, "%Y-%m-%d %H:%M:%S");
	gint64 now_unix = g_date_time_to_unix(time);
	g_date_time_unref(time);
	GtkListStore *down_store, *fin_store;
	guint num = g_thread_pool_get_num_threads(pool);
	if (num == 0) {
		if (!task_finish) {
			task_finish = TRUE;
			gtk_statusbar_push(status_bar, status_bar_id++, "就绪");
		}
		stop_thread = FALSE;
	} else {
		task_finish = FALSE;
		gchar *str;
		if (stop_thread == TRUE) {
			str = g_strdup_printf("已停止运行新线程,等待 %6u 完毕", num);
		} else {
			str = g_strdup_printf("%6u运行\t%6u待运行...", num,
					g_thread_pool_unprocessed(pool));
		}
		gtk_statusbar_push(status_bar, status_bar_id++, str);
		g_free(str);
	}
	g_mutex_lock(&notify_download_mutex);
	list = notify_download_list;
	down_store = my_download_ui_get_download_store(down_ui);
	fin_store = my_download_ui_get_finish_store(down_ui);
	while (list != NULL) {
		my_download_ui_mutex_lock(down_ui);
		task_msg = list->data;
		g_mutex_lock(&task_msg->mutex);
		if (task_msg->list_row == NULL) {
			gtk_list_store_append(down_store, &iter);
			path = gtk_tree_model_get_path(down_store, &iter);
			task_msg->list_row = gtk_tree_row_reference_new(down_store, path);
			ref = task_msg->list_row;
		} else {
			ref = task_msg->list_row;
			path = gtk_tree_row_reference_get_path(ref);
			gtk_tree_model_get_iter(down_store, &iter, path);
		}
		size_fmt = format_size_to_str(task_msg->content_size);
		switch (task_msg->GET_STATUS) {
		case Get_Begin:
			uri_fmt = soup_uri_to_string(task_msg->uri, FALSE);
			gtk_list_store_set(down_store, &iter, down_col_state, Wait,
					down_col_state_pixbuf,
					my_download_ui_get_download_state_pixbuf(down_ui, Wait),
					down_col_start_time, time_fmt, down_col_uri, uri_fmt, -1);
			g_free(uri_fmt);
			break;
		case Get_Head:
			gtk_list_store_set(down_store, &iter, down_col_name,
					task_msg->filename, down_col_file_size, size_fmt,
					down_col_save_local, task_msg->local, down_col_start_time,
					time_fmt, -1);
			break;
		case Get_Body:
			progress = 0;
			speed = (task_msg->dl_size - task_msg->pre_dlsize) * 4;
			task_msg->pre_dlsize = task_msg->dl_size;
			ela_time_fmt = format_size_to_str(speed);
			speed_fmt = g_strdup_printf("%s/s", ela_time_fmt);
			g_free(ela_time_fmt);
			if (task_msg->content_size > 0)
				progress = task_msg->dl_size * 100 / task_msg->content_size;
			time = g_date_time_new_from_unix_utc(
					now_unix - task_msg->start_time);
			ela_time_fmt = g_date_time_format(time, "%H:%M:%S");
			dlsize_size = format_dlsize_size(task_msg->dl_size,
					task_msg->content_size);
			gtk_list_store_set(down_store, &iter, down_col_progress, progress,
					down_col_size_dlsize, dlsize_size, down_col_elapsed,
					ela_time_fmt, down_col_state, Downloading,
					down_col_state_pixbuf,
					my_download_ui_get_download_state_pixbuf(down_ui,
							Downloading), down_col_speed, speed_fmt, -1);
			g_free(speed_fmt);
			g_free(dlsize_size);
			g_free(ela_time_fmt);
			g_date_time_unref(time);
			break;
		case Get_Finish:
		case Get_Cancel:
			finish_download_list = g_list_append(finish_download_list,
					task_msg);
			break;
		default:
			break;
		}
		g_free(size_fmt);
		gtk_tree_path_free(path);
		g_mutex_unlock(&task_msg->mutex);
		my_download_ui_mutex_unlock(down_ui);
		list = list->next;
	}
	list = finish_download_list;

	while (list != NULL) {
		my_download_ui_mutex_lock(down_ui);
		task_msg = list->data;
		g_print("Finish Task:%d %s\n", task_msg->id, task_msg->filename);
		notify_download_list = g_list_remove(notify_download_list, task_msg);
		path = gtk_tree_row_reference_get_path(task_msg->list_row);
		gtk_tree_model_get_iter(down_store, &iter, path);
		gtk_list_store_remove(down_store, &iter);
		gtk_list_store_append(fin_store, &iter);
		size_fmt = format_size_to_str(task_msg->content_size);
		uri_fmt = soup_uri_to_string(task_msg->uri, FALSE);
		if (task_msg->soup_status == SOUP_STATUS_OK) {
			state = Finish;
		} else {
			state = Error;
		}
		gtk_list_store_set(fin_store, &iter, finish_col_name,
				task_msg->filename, finish_col_uri, uri_fmt, finish_col_error,
				soup_status_get_phrase(task_msg->soup_status),
				finish_col_local, task_msg->local, finish_col_size,
				task_msg->content_size, finish_col_size_format, size_fmt,
				finish_col_state, state, finish_col_state_pixbuf,
				my_download_ui_get_download_state_pixbuf(down_ui, state),
				finish_col_finish_time, time_fmt, finish_col_finish_time_unix,
				now_unix, -1);
		my_download_ui_mutex_unlock(down_ui);
		g_free(uri_fmt);
		gtk_tree_path_free(path);
		//g_object_unref(task_msg);
		list = list->next;
	}
	g_list_free(finish_download_list);
	finish_download_list = NULL;
	g_mutex_unlock(&notify_download_mutex);
	g_free(time_fmt);
	return G_SOURCE_CONTINUE;
}
;

int main(int args, char *argv[]) {
	gtk_init(&args, &argv);
	my_app_init();
	MyMainui *ui = my_mainui_new();
	GtkLayout *layout = my_mainui_get_layout(ui);
	gtk_widget_show_all(ui);
	g_signal_connect_after(layout, "draw", layout_draw_link, NULL);
	g_signal_connect(ui, "delete-event", gtk_main_quit, NULL);
	g_signal_connect(ui, "add_child", main_ui_add, NULL);
	g_signal_connect(ui, "exec", main_ui_exec, NULL);
	g_signal_connect(ui, "open", main_ui_open, NULL);
	g_signal_connect(ui, "save", main_ui_save, NULL);
	g_signal_connect(ui, "setting", main_ui_setting, NULL);
	g_signal_connect(ui, "stop", main_ui_stop, NULL);
	g_signal_connect(ui, "down-info", main_ui_down_info, NULL);
	/*gchar *log_filename = g_strdup_printf("%s/log.txt", OUTPUT_DIR);
	log_file = g_io_channel_new_file(log_filename, "a+", NULL);
	g_free(log_filename);*/
	g_timeout_add(250, notify_thread_num, ui);
	gtk_main();
	return 0;
}
