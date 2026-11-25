/*
    tg
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Session files: the proposed filename and the safe replacement of the
 * destination. No GTK widgets here, so that it can be tested on its own. */

#include "tg.h"
#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Characters not allowed in filenames on Windows (and '/' everywhere) become
 * '_', as do control characters. Leading and trailing spaces are removed and
 * trailing dots or spaces replaced, which Windows would drop. Only the
 * filename is changed: the title stored in the session keeps its text.
 * Returns NULL if nothing usable is left. */
char *session_sanitize_filename_part(const char *text)
{
	if(!text) return NULL;
	char *s = g_strdup(text);
	g_strstrip(s);
	for(char *p = s; *p; p++)
		if((guchar)*p < 0x20 || (guchar)*p == 0x7f || strchr("\\/:*?\"<>|", *p))
			*p = '_';
	size_t length = strlen(s);
	while(length && (s[length - 1] == ' ' || s[length - 1] == '.'))
		s[--length] = '_';
	if(!length || !g_utf8_validate(s, -1, NULL)) {
		g_free(s);
		return NULL;
	}
	return s;
}

/* "YYYY-MM-DD name.tgj", or "YYYY-MM-DD.tgj" without a usable name.
 * date is "YYYY-MM-DD" (today, local time). Free with g_free(). */
char *session_suggest_filename(const char *date, const char *name)
{
	char *safe = session_sanitize_filename_part(name);
	char *s = safe ? g_strdup_printf("%s %s.tgj", date, safe) :
		g_strdup_printf("%s.tgj", date);
	g_free(safe);
	return s;
}

static int flush_to_disk(FILE *f)
{
	if(fflush(f)) return 1;
#ifdef _WIN32
	return _commit(_fileno(f)) != 0;
#else
	return fsync(fileno(f)) != 0;
#endif
}

/* Replace `to` by `from` in one step. The previous `to` stays in place if it fails. */
static int replace_file(const char *from, const char *to)
{
#ifdef _WIN32
	wchar_t *wfrom = g_utf8_to_utf16(from, -1, NULL, NULL, NULL);
	wchar_t *wto = g_utf8_to_utf16(to, -1, NULL, NULL, NULL);
	int ok = wfrom && wto && MoveFileExW(wfrom, wto,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	g_free(wfrom);
	g_free(wto);
	return !ok;
#else
	return rename(from, to) != 0;
#endif
}

/* Write a file through `writer` into a temporary file next to `path`, check
 * that everything was written and closed, and only then replace `path` with
 * it. On any failure the temporary file is removed and `path` is left as it
 * was. `path` is UTF-8 (GLib filename encoding on Windows). */
int session_write_file_safely(const char *path, int (*writer)(FILE *f, void *data), void *data)
{
	if(!path || !*path || !writer) return 1;
	char *tmp = g_strconcat(path, ".tmp-XXXXXX", NULL);
	int fd = g_mkstemp_full(tmp, O_RDWR | O_BINARY, 0666);
	if(fd < 0) {
		g_free(tmp);
		return 1;
	}
	FILE *f = fdopen(fd, "wb");
	if(!f) {
		close(fd);
		g_remove(tmp);
		g_free(tmp);
		return 1;
	}
	int failed = writer(f, data);
	if(!failed && (ferror(f) || flush_to_disk(f)))
		failed = 1;
	if(fclose(f))
		failed = 1;
	if(!failed && replace_file(tmp, path))
		failed = 1;
	if(failed)
		g_remove(tmp);
	g_free(tmp);
	return failed;
}
