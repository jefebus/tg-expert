/* TG-30: proposed session filename and safe replacement of the destination.
 *
 * Build: it includes src/session.c and only needs GLib (and the GTK headers
 * pulled in by tg.h), e.g. in MSYS2:
 *   gcc -o session_file tests/session_file.c $(pkg-config --cflags --libs gtk+-3.0) */
#include <assert.h>
#include <glib/gstdio.h>

#include "../src/session.c"

#define CHECK(X) do { if(!(X)) { fprintf(stderr, "line %d: %s\n", __LINE__, #X); abort(); } } while(0)

static int same_text(char *s, const char *expected)
{
	int r = s && !strcmp(s, expected);
	g_free(s);
	return r;
}

static void test_names(void)
{
	CHECK(same_text(session_suggest_filename("2026-11-25", "Revisión Omega"),
		"2026-11-25 Revisión Omega.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", NULL), "2026-11-25.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", "   "), "2026-11-25.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", "a/b\\c:d*e?f\"g<h>i|j"),
		"2026-11-25 a_b_c_d_e_f_g_h_i_j.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", "line\nbreak\ttab"),
		"2026-11-25 line_break_tab.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", "  name. "), "2026-11-25 name_.tgj"));
	CHECK(same_text(session_suggest_filename("2026-11-25", "Reloj \xE2\x82\xAC \xF0\x9F\x95\x90"),
		"2026-11-25 Reloj \xE2\x82\xAC \xF0\x9F\x95\x90.tgj"));
	CHECK(session_sanitize_filename_part(NULL) == NULL);
	CHECK(session_sanitize_filename_part("") == NULL);
	CHECK(same_text(session_sanitize_filename_part("..."), "___"));
}

static int write_text(FILE *f, void *data)
{
	return fputs(data, f) == EOF;
}

static int write_and_fail(FILE *f, void *data)
{
	fputs(data, f);
	return 1;
}

static char *read_all(const char *path)
{
	char *s = NULL;
	if(!g_file_get_contents(path, &s, NULL, NULL)) return NULL;
	return s;
}

static int count_entries(const char *dir)
{
	GDir *d = g_dir_open(dir, 0, NULL);
	CHECK(d);
	int n = 0;
	while(g_dir_read_name(d)) n++;
	g_dir_close(d);
	return n;
}

static void test_safe_write(void)
{
	char *dir = g_dir_make_tmp("tg-session-XXXXXX", NULL);
	CHECK(dir);
	/* Spaces and non-ASCII characters in the name */
	char *path = g_build_filename(dir, "2026-11-25 Revisión ñ €.tgj", NULL);

	CHECK(session_write_file_safely(path, write_text, "first") == 0);
	CHECK(same_text(read_all(path), "first"));
	CHECK(count_entries(dir) == 1);

	/* Replacing an existing file */
	CHECK(session_write_file_safely(path, write_text, "second") == 0);
	CHECK(same_text(read_all(path), "second"));
	CHECK(count_entries(dir) == 1);

	/* A failed write keeps the previous file and leaves no temporary file */
	CHECK(session_write_file_safely(path, write_and_fail, "broken") != 0);
	CHECK(same_text(read_all(path), "second"));
	CHECK(count_entries(dir) == 1);

	/* A destination in a folder that does not exist fails cleanly */
	char *missing = g_build_filename(dir, "no-such-folder", "x.tgj", NULL);
	CHECK(session_write_file_safely(missing, write_text, "x") != 0);
	CHECK(count_entries(dir) == 1);
	g_free(missing);

	/* The destination cannot be replaced (it is a folder): the folder stays */
	char *folder = g_build_filename(dir, "folder.tgj", NULL);
	CHECK(g_mkdir(folder, 0700) == 0);
	CHECK(session_write_file_safely(folder, write_text, "x") != 0);
	CHECK(g_file_test(folder, G_FILE_TEST_IS_DIR));
	CHECK(count_entries(dir) == 2);
	g_rmdir(folder);
	g_free(folder);

	CHECK(session_write_file_safely(NULL, write_text, "x") != 0);
	CHECK(session_write_file_safely("", write_text, "x") != 0);

	g_remove(path);
	g_rmdir(dir);
	g_free(path);
	g_free(dir);
}

int main(void)
{
	test_names();
	test_safe_write();
	return 0;
}
