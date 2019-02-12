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
	OUTPUT_DIR = g_strdup_printf("easy_crawler/%s", time_str);
	g_mkdir_with_parents(OUTPUT_DIR, 0777);
	COOKIE_FILE = g_strdup_printf("%s%scookies.txt", OUTPUT_DIR,
			G_DIR_SEPARATOR_S);
	session = soup_session_new();
	g_object_set(session, "timeout", 20, NULL);
	soup_session_add_feature(session,
			soup_cookie_jar_text_new(COOKIE_FILE, FALSE));

	operater_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_linklist = g_hash_table_new(g_direct_hash, g_direct_equal);

	log_task_href = NULL;
	MAX_THREAD = 10;

	down_ui = my_download_ui_new();
	mycurl = my_curl_new(down_ui);
	my_curl_set_set_cookies_callback(mycurl, task_curl_set_cookies_callback,
			my_task_message_free);
	my_curl_set_get_cookies_callback(mycurl, task_my_curl_get_cookies_callback);
	my_curl_set_set_filename_callback(mycurl, task_curl_set_filename_callback,
			my_task_message_free);
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
	meta_charset = g_regex_new("<meta.*charset=.*>", 0, 0, NULL);

	log_mutex = g_mutex_new();
	g_mutex_init(log_mutex);
	log_task_href_mutex = g_mutex_new();
	g_mutex_init(log_task_href_mutex);
	g_mutex_init(&notify_download_mutex);
	notify_download_list = NULL;
	finish_download_list = NULL;
	process_queue = g_async_queue_new();
	g_free(time_str);
}

gboolean notify_thread_num(MyMainui *ui) {
	guint down_count = 0;
	GtkTreeIter iter;
	MyTaskMessage *task_msg;
	GtkStatusbar *status_bar = my_mainui_get_statusbar(ui);
	GtkListStore *down_store = my_download_ui_get_download_store(down_ui);
	GString *str = g_string_new("");
	if (gtk_tree_model_get_iter_first(down_store, &iter)) {
		down_count++;
		while(gtk_tree_model_iter_next(down_store,&iter)){
			down_count++;
		}
	};
	while (g_async_queue_length(process_queue) > 0 && runing_count < MAX_THREAD) {
		task_msg = g_async_queue_pop(process_queue);
		g_idle_add(task_source, task_msg);
		runing_count++;
	}
	if (runing_count == 0) {
		task_finish = TRUE;
		g_string_append(str, "就绪");
		stop_thread = FALSE;
	} else {
		task_finish = FALSE;
		if (stop_thread == TRUE) {
			g_string_append_printf(str, "等待 %8u 任务完毕", runing_count);
		} else {
			g_string_append_printf(str, "%8u 任务运行|%8d 等待运行...", runing_count,
					g_async_queue_length(process_queue));
		}
	}
	if (down_count > 0)
		g_string_append_printf(str, "\t%8d正在下载", down_count);
	gtk_statusbar_push(status_bar, status_bar_id++, str->str);
	g_string_free(str, TRUE);
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
	g_timeout_add(250, notify_thread_num, ui);
	gtk_main();
	return 0;
}
