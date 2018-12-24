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
	session = soup_session_new();
	operater_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_opdata = g_hash_table_new(g_direct_hash, g_direct_equal);
	task_to_linklist = g_hash_table_new(g_direct_hash, g_direct_equal);
	log_task_href=NULL;
}

gboolean notify_thread_num(MyMainui *ui) {
	GtkStatusbar *status_bar = my_mainui_get_statusbar(ui);
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
			str = g_strdup_printf("%6u运行\t%6u待运行...", num,g_thread_pool_unprocessed(pool));
		}
		gtk_statusbar_push(status_bar, status_bar_id++, str);
		g_free(str);
	}
	return G_SOURCE_CONTINUE;
}
;

int main(int args, char *argv[]) {
	gtk_init(&args, &argv);
	my_app_init();
	MyMainui *ui = my_mainui_new();
	GtkLayout *layout = my_mainui_get_layout(ui);
	GDateTime *time = g_date_time_new_now_local();
	gchar *time_str=g_date_time_format(time, "%Y-%m-%d_%H-%M-%S");
	gtk_widget_show_all(ui);
	g_signal_connect_after(layout, "draw", layout_draw_link, NULL);
	g_signal_connect(ui, "delete-event", gtk_main_quit, NULL);
	g_signal_connect(ui, "add_child", main_ui_add, NULL);
	g_signal_connect(ui, "exec", main_ui_exec, NULL);
	g_signal_connect(ui, "open", main_ui_open, NULL);
	g_signal_connect(ui, "save", main_ui_save, NULL);
	g_signal_connect(ui, "setting", main_ui_setting, NULL);
	g_signal_connect(ui, "stop", main_ui_stop, NULL);
	OUTPUT_DIR = g_strdup_printf("easy_crawler/%s",time_str);
	g_free(time_str);
	g_mkdir_with_parents(OUTPUT_DIR, 0777);
	pool = g_thread_pool_new(task_thread, NULL, 10,
	FALSE, NULL);
	COOKIE_FILE = NULL;
	log_mutex=g_mutex_new();
	g_mutex_init(log_mutex);
	log_task_href_mutex=g_mutex_new();
	g_mutex_init(log_task_href_mutex);
	gchar *log_filename=g_strdup_printf("%s/log.txt",OUTPUT_DIR);
	log_file=g_io_channel_new_file(log_filename,"a+",NULL);
	g_free(log_filename);
	g_timeout_add_seconds(1, notify_thread_num, ui);
	g_date_time_unref(time);
	gtk_main();
	return 0;
}
