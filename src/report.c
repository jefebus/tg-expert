/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-16: the PDF report, written directly with Cairo's PDF backend.
 *
 * The report is drawn from a copy of the session data (struct report_data),
 * taken when the export starts, with the graph renderers shared with the
 * screen (TG-31). Text is set with PangoCairo, so that it wraps, keeps
 * Unicode and stays selectable. Everything is measured in PDF points on A4.
 *
 * Every page has a running header (logo, program, report title, job
 * reference and date, a teal rule) and a footer. Contents, each part only
 * if it has something:
 *  - first page: a panel with the project name, job reference, watch, last
 *    service, owner (only when asked for) and report date, a note that it is
 *    not a certificate, the general notes and a table of the runs;
 *  - each run, in the order recorded, from a new page: its period, notes,
 *    Trends (statistics, graphs, correlations), its final state and its
 *    snapshots, each on a new page with its own notes first, then its
 *    readings, paper strip and waveforms;
 *  - current state: the real-time tab frozen at export time;
 *  - other snapshots: those of older files, without a run.
 * Notes are Markdown (a subset): paragraphs, line breaks, headings (#, ##,
 * ###), **bold**, *italic* and _italic_, lists (-, *, +, 1.). Anything else
 * is shown as written: no text is ever dropped. */

#include "tg.h"
#include <pango/pangocairo.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif

/* Page and type as in the user manual (docs/manual/lib.typ): A4, margins of
 * 18 mm across and 17 mm down, DejaVu Sans 9.3 pt, ink text, navy titles,
 * teal subheadings, navy table heads, grey footer under a thin line */
#define MM 2.834646	// points per mm
#define PAGE_W 595.276	// A4 in points
#define PAGE_H 841.89
#define MARGIN_X (18 * MM)
/* The running header, on every page: logo, product name, report title */
#define HEADER_TOP (10 * MM)
#define HEADER_H 30.0
#define HEADER_RULE_Y (HEADER_TOP + HEADER_H + 6)
#define MARGIN_TOP (HEADER_RULE_Y + 16)	// where the body starts
#define FOOTER_LINE_Y (PAGE_H - 17 * MM)	// the footer sits in the bottom margin
#define BODY_BOTTOM (FOOTER_LINE_Y - 10)
#define CONTENT_W (PAGE_W - 2 * MARGIN_X)

#define FONT_FAMILY "DejaVu Sans, Sans"	// as the manual; any sans if it is missing
#define SIZE_H1 16
#define SIZE_H2 11
#define SIZE_H3 10
#define SIZE_BODY 9.3
#define SIZE_SMALL 8
#define SIZE_GRAPH 7.5	// text inside the graphs

#define MEASUREMENT_HEIGHT 420	// paper strip and waveforms of a snapshot
#define TREND_GRAPHS_HEIGHT 300

/* Colours of the text and tables. The report's own headings are navy and
 * teal; headings written in the notes (Markdown) are black, so that the two
 * are not confused. In monochrome everything is black and grey. */
struct report_style {
	struct render_rgb ink;		// body text
	struct render_rgb black;	// headings of the notes
	struct render_rgb title;	// report title, section headings, labels (navy)
	struct render_rgb sub;		// subheadings (teal)
	struct render_rgb muted;	// footer and secondary lines
	struct render_rgb line;		// rules and table borders
	struct render_rgb head_fill;	// table heads
	struct render_rgb head_text;
	struct render_rgb accent;	// the beat trace of the logo, on the navy tile
	struct render_rgb panel;	// background of the information panel (the manual's notes)
};

static const struct report_style style_colour = {
	.ink = {0x24 / 255., 0x37 / 255., 0x46 / 255.},		// #243746
	.black = {0, 0, 0},
	.title = {0x17 / 255., 0x32 / 255., 0x4D / 255.},	// #17324D
	.sub = {0x14 / 255., 0x7D / 255., 0x83 / 255.},		// #147D83
	.muted = {90 / 255., 90 / 255., 90 / 255.},		// luma(90)
	.line = {0xC8 / 255., 0xD5 / 255., 0xDA / 255.},	// #C8D5DA
	.head_fill = {0x17 / 255., 0x32 / 255., 0x4D / 255.},
	.head_text = {1, 1, 1},
	.accent = {0x5F / 255., 0xC4 / 255., 0xC9 / 255.},	// a light teal
	.panel = {0xEA / 255., 0xF3 / 255., 0xF4 / 255.}	// #EAF3F4
};

static const struct report_style style_mono = {
	.ink = {0, 0, 0},
	.black = {0, 0, 0},
	.title = {0, 0, 0},
	.sub = {0, 0, 0},
	.muted = {90 / 255., 90 / 255., 90 / 255.},
	.line = {.75, .75, .75},
	.head_fill = {.25, .25, .25},
	.head_text = {1, 1, 1},
	.accent = {1, 1, 1},
	.panel = {.94, .94, .94}
};

struct report {
	cairo_t *cr;
	const struct report_data *d;
	const struct render_theme *th;
	const struct report_style *st;
	double y;	// where the next block starts
	int page;	// 1, 2...
	int pages;	// from the first pass, 0 = not known yet
	int blank;	// nothing drawn on this page yet
};

static int text_set(const char *s)
{
	return s && *s;
}

/* ---- Pages ---- */

static PangoLayout *layout_new(struct report *r, double size, int bold, int italic, double width)
{
	PangoLayout *l = pango_cairo_create_layout(r->cr);
	// One unit is one point: the sizes below are point sizes
	pango_cairo_context_set_resolution(pango_layout_get_context(l), 72);
	pango_layout_context_changed(l);
	PangoFontDescription *f = pango_font_description_from_string(FONT_FAMILY);
	pango_font_description_set_size(f, (int)(size * PANGO_SCALE));
	if(bold) pango_font_description_set_weight(f, PANGO_WEIGHT_BOLD);
	if(italic) pango_font_description_set_style(f, PANGO_STYLE_ITALIC);
	pango_layout_set_font_description(l, f);
	pango_font_description_free(f);
	if(width > 0) {
		pango_layout_set_width(l, (int)(width * PANGO_SCALE));
		pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
	}
	return l;
}

/* Markup if it is valid, else the text as written (nothing is dropped) */
static void layout_set(PangoLayout *l, const char *markup, const char *raw)
{
	if(markup && pango_parse_markup(markup, -1, 0, NULL, NULL, NULL, NULL))
		pango_layout_set_markup(l, markup, -1);
	else
		pango_layout_set_text(l, raw ? raw : "", -1);
}

/* As in the manual: a thin line, then the program and date at the left and
 * the page at the right */
static void footer(struct report *r)
{
	cairo_set_line_width(r->cr, .5);
	render_set_rgb(r->cr, (struct render_rgb){210 / 255., 210 / 255., 210 / 255.}, 1);
	cairo_move_to(r->cr, MARGIN_X, FOOTER_LINE_Y);
	cairo_line_to(r->cr, MARGIN_X + CONTENT_W, FOOTER_LINE_Y);
	cairo_stroke(r->cr);

	/* TRANSLATORS: footer of every page of the PDF report, at the left:
	 * program name, version, and date and time of the export */
	char *left = g_strdup_printf(_("Generated with %s %s \xC2\xB7 %s"),
		PROGRAM_NAME, VERSION, r->d->date ? r->d->date : "");
	/* TRANSLATORS: footer of every page of the PDF report, at the right */
	char *right = g_strdup_printf(_("page %d of %d"), r->page, r->pages > 0 ? r->pages : r->page);
	render_set_rgb(r->cr, r->st->muted, 1);
	PangoLayout *l = layout_new(r, SIZE_SMALL, 0, 0, CONTENT_W);
	pango_layout_set_text(l, left, -1);
	cairo_move_to(r->cr, MARGIN_X, FOOTER_LINE_Y + 3);
	pango_cairo_show_layout(r->cr, l);
	pango_layout_set_alignment(l, PANGO_ALIGN_RIGHT);
	pango_layout_set_text(l, right, -1);
	cairo_move_to(r->cr, MARGIN_X, FOOTER_LINE_Y + 3);
	pango_cairo_show_layout(r->cr, l);
	g_object_unref(l);
	g_free(left);
	g_free(right);
}

/* The report title: the project name, else the watch, else a generic title */
static const char *report_title(const struct report_data *d)
{
	if(text_set(d->project.title)) return d->project.title;
	if(text_set(d->project.watch_label)) return d->project.watch_label;
	/* TRANSLATORS: title of the PDF report when the project has no name */
	return _("Timegrapher report");
}

static void rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r)
{
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
	cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
	cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
	cairo_close_path(cr);
}

/* Used only when the program icon cannot be loaded (struct report_data
 * logo NULL): a watch at ten past ten on a navy tile, with the beat trace
 * of a timegrapher under it */
static void draw_logo(cairo_t *cr, const struct report_style *st, double x, double y, double s)
{
	cairo_save(cr);
	cairo_new_path(cr);
	rounded_rect(cr, x, y, s, s, s * 0.18);
	render_set_rgb(cr, st->title, 1);
	cairo_fill(cr);

	const double cx = x + s / 2, cy = y + s * 0.44, R = s * 0.25;
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	// Crown and case
	cairo_rectangle(cr, cx - s * 0.05, cy - R - s * 0.10, s * 0.10, s * 0.07);
	cairo_fill(cr);
	cairo_set_line_width(cr, s * 0.055);
	cairo_arc(cr, cx, cy, R, 0, 2 * M_PI);
	cairo_stroke(cr);
	// Hands at ten past ten, and the centre
	cairo_set_line_width(cr, s * 0.045);
	cairo_move_to(cr, cx, cy);
	// Hour hand towards 10, minute hand towards 2 (y grows downwards)
	cairo_line_to(cr, cx + R * 0.55 * cos(-5 * M_PI / 6), cy + R * 0.55 * sin(-5 * M_PI / 6));
	cairo_move_to(cr, cx, cy);
	cairo_line_to(cr, cx + R * 0.75 * cos(-M_PI / 6), cy + R * 0.75 * sin(-M_PI / 6));
	cairo_stroke(cr);
	cairo_arc(cr, cx, cy, s * 0.035, 0, 2 * M_PI);
	cairo_fill(cr);
	// The beat trace: a flat line with a tic and a toc
	const double by = y + s * 0.85, x0 = x + s * 0.14, x1 = x + s * 0.86, w = x1 - x0;
	cairo_set_line_width(cr, s * 0.04);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_move_to(cr, x0, by);
	cairo_line_to(cr, x0 + w * 0.22, by);
	cairo_line_to(cr, x0 + w * 0.28, by - s * 0.09);
	cairo_line_to(cr, x0 + w * 0.34, by + s * 0.04);
	cairo_line_to(cr, x0 + w * 0.40, by);
	cairo_line_to(cr, x0 + w * 0.62, by);
	cairo_line_to(cr, x0 + w * 0.68, by - s * 0.07);
	cairo_line_to(cr, x0 + w * 0.74, by + s * 0.03);
	cairo_line_to(cr, x0 + w * 0.80, by);
	cairo_line_to(cr, x1, by);
	render_set_rgb(cr, st->accent, 1);
	cairo_stroke(cr);
	cairo_restore(cr);
}

/* The running header of every page: logo and product at the left, the
 * report title with the job reference and date at the right, a teal rule */
static void header(struct report *r)
{
	const struct report_data *d = r->d;
	if(d->logo) {
		// The program icon, scaled into the square (it keeps its proportions)
		int iw = gdk_pixbuf_get_width(d->logo), ih = gdk_pixbuf_get_height(d->logo);
		double k = HEADER_H / fmax(iw, ih);
		cairo_save(r->cr);
		cairo_translate(r->cr, MARGIN_X + (HEADER_H - iw * k) / 2, HEADER_TOP + (HEADER_H - ih * k) / 2);
		cairo_scale(r->cr, k, k);
		gdk_cairo_set_source_pixbuf(r->cr, d->logo, 0, 0);
		cairo_pattern_set_filter(cairo_get_source(r->cr), CAIRO_FILTER_BEST);
		cairo_paint(r->cr);
		cairo_restore(r->cr);
	} else
		draw_logo(r->cr, r->st, MARGIN_X, HEADER_TOP, HEADER_H);

	const double tx = MARGIN_X + HEADER_H + 9;
	PangoLayout *l = layout_new(r, 11, 1, 0, 0);
	pango_layout_set_text(l, PROGRAM_NAME, -1);
	render_set_rgb(r->cr, r->st->title, 1);
	cairo_move_to(r->cr, tx, HEADER_TOP + 1);
	pango_cairo_show_layout(r->cr, l);
	g_object_unref(l);
	l = layout_new(r, SIZE_SMALL, 0, 0, 0);
	/* TRANSLATORS: under the program name in the header of every page of the PDF report */
	pango_layout_set_text(l, _("Timegrapher measurement report"), -1);
	render_set_rgb(r->cr, r->st->sub, 1);
	cairo_move_to(r->cr, tx, HEADER_TOP + 17);
	pango_cairo_show_layout(r->cr, l);
	g_object_unref(l);

	// At the right, shortened with "…" if it is too long
	const double rw = CONTENT_W * 0.55, rx = MARGIN_X + CONTENT_W - rw;
	l = layout_new(r, 11, 1, 0, rw);
	pango_layout_set_alignment(l, PANGO_ALIGN_RIGHT);
	pango_layout_set_ellipsize(l, PANGO_ELLIPSIZE_END);
	pango_layout_set_text(l, report_title(d), -1);
	render_set_rgb(r->cr, r->st->ink, 1);
	cairo_move_to(r->cr, rx, HEADER_TOP + 1);
	pango_cairo_show_layout(r->cr, l);
	g_object_unref(l);
	char *meta = text_set(d->project.project_reference) ?
		g_strdup_printf("%s \xC2\xB7 %s", d->project.project_reference, d->date ? d->date : "") :
		g_strdup(d->date ? d->date : "");
	l = layout_new(r, SIZE_SMALL, 0, 0, rw);
	pango_layout_set_alignment(l, PANGO_ALIGN_RIGHT);
	pango_layout_set_ellipsize(l, PANGO_ELLIPSIZE_END);
	pango_layout_set_text(l, meta, -1);
	render_set_rgb(r->cr, r->st->muted, 1);
	cairo_move_to(r->cr, rx, HEADER_TOP + 17);
	pango_cairo_show_layout(r->cr, l);
	g_object_unref(l);
	g_free(meta);

	cairo_set_line_width(r->cr, 1.2);
	render_set_rgb(r->cr, r->st->sub, 1);
	cairo_move_to(r->cr, MARGIN_X, HEADER_RULE_Y);
	cairo_line_to(r->cr, MARGIN_X + CONTENT_W, HEADER_RULE_Y);
	cairo_stroke(r->cr);
}

static void new_page(struct report *r)
{
	footer(r);
	cairo_show_page(r->cr);
	r->page++;
	header(r);
	r->y = MARGIN_TOP;
	r->blank = 1;
}

/* Room for h points from r->y, or a new page. A block taller than a page
 * is drawn on a fresh page anyway. */
static void ensure(struct report *r, double h)
{
	if(!r->blank && r->y + h > BODY_BOTTOM)
		new_page(r);
	r->blank = 0;
}

/* The lines of a layout from r->y, going to a new page between lines when
 * needed, so that long text continues on the next page */
static void show_lines(struct report *r, PangoLayout *l, double x, struct render_rgb col, double alpha)
{
	int n = pango_layout_get_line_count(l);
	// Lines are placed one by one, so the layout's spacing is added here
	double spacing = pango_layout_get_spacing(l) / (double)PANGO_SCALE;
	for(int i = 0; i < n; i++) {
		PangoLayoutLine *line = pango_layout_get_line_readonly(l, i);
		PangoRectangle logical;
		pango_layout_line_get_extents(line, NULL, &logical);
		double h = logical.height / (double)PANGO_SCALE;
		double base = -logical.y / (double)PANGO_SCALE;
		ensure(r, h);
		render_set_rgb(r->cr, col, alpha); // the footer of a new page changed it
		cairo_move_to(r->cr, x + logical.x / (double)PANGO_SCALE, r->y + base);
		pango_cairo_show_layout_line(r->cr, line);
		r->y += h + (i + 1 < n ? spacing : 0);
	}
}

static void text_block_col(struct report *r, const char *markup, const char *raw, double size,
	int bold, int italic, double indent, struct render_rgb col, double alpha, double after)
{
	PangoLayout *l = layout_new(r, size, bold, italic, CONTENT_W - indent);
	// The manual's line spacing (leading 1.1 em)
	pango_layout_set_spacing(l, (int)(size * 0.25 * PANGO_SCALE));
	layout_set(l, markup, raw);
	show_lines(r, l, MARGIN_X + indent, col, alpha);
	g_object_unref(l);
	r->y += after;
}

static void text_block(struct report *r, const char *markup, const char *raw, double size,
	int bold, int italic, double indent, double alpha, double after)
{
	text_block_col(r, markup, raw, size, bold, italic, indent, r->st->ink, alpha, after);
}

/* Secondary lines (source, position, notices): grey as in the manual's captions */
static void muted(struct report *r, const char *text, int italic, double after)
{
	text_block_col(r, NULL, text, SIZE_SMALL, 0, italic, 0, r->st->muted, 1, after);
}

/* A heading kept on the same page as the first keep points after it */
static void heading_markup(struct report *r, const char *markup, const char *raw, double size,
	struct render_rgb col, double before, double keep)
{
	if(!r->blank) r->y += before;
	PangoLayout *l = layout_new(r, size, 1, 0, CONTENT_W);
	layout_set(l, markup, raw);
	int w, h;
	pango_layout_get_size(l, &w, &h);
	ensure(r, h / (double)PANGO_SCALE + keep);
	show_lines(r, l, MARGIN_X, col, 1);
	g_object_unref(l);
	r->y += size * 0.4;
}

/* The report's own headings: level 1 navy, the others teal */
static void heading(struct report *r, const char *text, double size, double before, double keep)
{
	heading_markup(r, NULL, text, size, size >= SIZE_H1 ? r->st->title : r->st->sub, before, keep);
}

static void rule(struct report *r, double after)
{
	ensure(r, 1);
	cairo_set_line_width(r->cr, .5);
	render_set_rgb(r->cr, r->st->line, 1);
	cairo_move_to(r->cr, MARGIN_X, r->y + .5);
	cairo_line_to(r->cr, MARGIN_X + CONTENT_W, r->y + .5);
	cairo_stroke(r->cr);
	r->y += after;
}

/* ---- Markdown ---- */

static void append_escaped(GString *out, const char *s, size_t n)
{
	if(!n) return;
	char *e = g_markup_escape_text(s, (gssize)n);
	g_string_append(out, e);
	g_free(e);
}

/* Letters and digits, and any byte of a non-ASCII character */
static int word_byte(unsigned char c)
{
	return c >= 0x80 || g_ascii_isalnum(c);
}

/* Where the marker opened at s[open] (mlen characters c) closes, or 0. The
 * content must not be empty nor start or end with a space; _ only opens and
 * closes at word boundaries, so that names like snake_case stay as written. */
static size_t md_close(const char *s, size_t n, size_t open, size_t mlen, char c)
{
	size_t first = open + mlen;
	if(first >= n || s[first] == ' ' || s[first] == '\n' || s[first] == '\t') return 0;
	if(c == '_' && open > 0 && word_byte((unsigned char)s[open - 1])) return 0;
	for(size_t j = first + 1; j + mlen <= n; j++) {
		if(s[j] != c) continue;
		if(mlen == 1 && c == '*' && j + 1 < n && s[j + 1] == '*') {
			j++;	// a ** inside *...* belongs to a bold part
			continue;
		}
		if(mlen == 2 && s[j + 1] != c) continue;
		if(s[j - 1] == ' ' || s[j - 1] == '\n' || s[j - 1] == '\t') continue;
		if(c == '_' && j + 1 < n && word_byte((unsigned char)s[j + 1])) continue;
		return j;
	}
	return 0;
}

/* **bold**, *italic* and _italic_ to Pango markup; the rest escaped */
static void md_inline(GString *out, const char *s, size_t n)
{
	size_t run = 0, i = 0;
	while(i < n) {
		char c = s[i];
		if(c != '*' && c != '_') {
			i++;
			continue;
		}
		size_t mlen = c == '*' && i + 1 < n && s[i + 1] == '*' ? 2 : 1;
		size_t close = md_close(s, n, i, mlen, c);
		if(!close) {
			i += mlen;	// shown as written
			continue;
		}
		append_escaped(out, s + run, i - run);
		g_string_append(out, mlen == 2 ? "<b>" : "<i>");
		md_inline(out, s + i + mlen, close - i - mlen);
		g_string_append(out, mlen == 2 ? "</b>" : "</i>");
		i = close + mlen;
		run = i;
	}
	append_escaped(out, s + run, n - run);
}

static char *md_markup(const char *s)
{
	GString *out = g_string_new(NULL);
	md_inline(out, s, strlen(s));
	return g_string_free(out, FALSE);
}

enum md_block { MD_NONE, MD_PARAGRAPH, MD_ITEM };

static void md_flush(struct report *r, GString *text, enum md_block *kind, const char *marker)
{
	if(*kind == MD_NONE || !text->len) {
		*kind = MD_NONE;
		g_string_truncate(text, 0);
		return;
	}
	char *markup = md_markup(text->str);
	if(*kind == MD_PARAGRAPH) {
		text_block(r, markup, text->str, SIZE_BODY, 0, 0, 0, 1, 4);
	} else {
		// A list item: the marker in the hanging indent, on the first line
		const double indent = 18;
		PangoLayout *l = layout_new(r, SIZE_BODY, 0, 0, CONTENT_W - indent);
		layout_set(l, markup, text->str);
		PangoLayoutLine *first = pango_layout_get_line_readonly(l, 0);
		if(first) {
			PangoRectangle logical;
			pango_layout_line_get_extents(first, NULL, &logical);
			ensure(r, logical.height / (double)PANGO_SCALE);
			PangoLayout *m = layout_new(r, SIZE_BODY, 0, 0, 0);
			pango_layout_set_text(m, marker, -1);
			render_set_rgb(r->cr, r->st->ink, 1);
			cairo_move_to(r->cr, MARGIN_X + 5, r->y);
			pango_cairo_show_layout(r->cr, m);
			g_object_unref(m);
		}
		show_lines(r, l, MARGIN_X + indent, r->st->ink, 1);
		g_object_unref(l);
		r->y += 2;
	}
	g_free(markup);
	*kind = MD_NONE;
	g_string_truncate(text, 0);
}

/* A list item marker at p: "-", "*" or "+" then a space, or a number then
 * "." or ")" and a space. Writes what to show and returns the text after it. */
static const char *md_item(const char *p, char *marker, size_t size)
{
	if((*p == '-' || *p == '*' || *p == '+') && p[1] == ' ') {
		snprintf(marker, size, "\xE2\x80\xA2");	// bullet
		p += 2;
	} else {
		const char *q = p;
		while(*q >= '0' && *q <= '9' && q - p < 9) q++;
		if(q == p || (*q != '.' && *q != ')') || q[1] != ' ')
			return NULL;
		snprintf(marker, size, "%.*s.", (int)(q - p), p);
		p = q + 2;
	}
	while(*p == ' ') p++;
	return *p ? p : NULL;
}

static void markdown(struct report *r, const char *text)
{
	gchar **lines = g_strsplit(text, "\n", -1);
	GString *block = g_string_new(NULL);
	enum md_block kind = MD_NONE;
	char marker[16] = "";
	for(int i = 0; lines[i]; i++) {
		char *line = lines[i];
		size_t len = strlen(line);
		if(len && line[len - 1] == '\r') line[--len] = 0;
		const char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(!*p) {	// a blank line ends the block
			md_flush(r, block, &kind, marker);
			continue;
		}
		// Headings: #, ## or ### and a space, indented by 3 spaces at most
		int hashes = 0;
		while(p[hashes] == '#') hashes++;
		if(hashes >= 1 && hashes <= 3 && p[hashes] == ' ' && p - line <= 3) {
			const char *h = p + hashes;
			while(*h == ' ') h++;
			if(*h) {
				md_flush(r, block, &kind, marker);
				char *markup = md_markup(h);
				// In black: the report's own headings are navy and teal
				heading_markup(r, markup, h, hashes == 1 ? SIZE_H2 : hashes == 2 ? SIZE_H3 : SIZE_BODY + .5,
					r->st->black, 6, 2 * SIZE_BODY);
				g_free(markup);
				continue;
			}
		}
		char item_marker[16];
		const char *item = md_item(p, item_marker, sizeof(item_marker));
		if(item) {
			md_flush(r, block, &kind, marker);
			kind = MD_ITEM;
			snprintf(marker, sizeof(marker), "%s", item_marker);
			g_string_assign(block, item);
			continue;
		}
		// Another line of the same paragraph or item: the line break is kept
		if(kind == MD_NONE)
			kind = MD_PARAGRAPH;
		else
			g_string_append_c(block, '\n');
		g_string_append(block, p);
	}
	md_flush(r, block, &kind, marker);
	g_string_free(block, TRUE);
	g_strfreev(lines);
	r->y += 4;
}

/* ---- Blocks ---- */

/* The project details in a panel like the manual's notes: pale background,
 * a teal bar at the left, labels in navy and values beside them */
static void info_panel(struct report *r)
{
	const struct project_metadata *m = &r->d->project;
	const char *labels[6], *values[6];
	int n = 0;
#define ROW(L, V) if(text_set(V)) { labels[n] = (L); values[n++] = (V); }
	ROW(_("Project name"), m->title);
	ROW(_("Job reference"), m->project_reference);
	ROW(_("Watch"), m->watch_label);
	ROW(_("Last service"), m->last_service_date);
	if(r->d->include_owner)
		ROW(_("Owner"), m->owner);
	/* TRANSLATORS: PDF report, in the panel with the project details: when it was made */
	ROW(_("Report date"), r->d->date);
#undef ROW
	if(!n) return;

	const double pad = 9, bar = 2.5, label_w = 118;
	const double value_x = MARGIN_X + bar + pad + label_w;
	const double value_w = MARGIN_X + CONTENT_W - pad - value_x;
	PangoLayout *lab[6], *val[6];
	double rows[6], h = 2 * pad - 4;
	for(int i = 0; i < n; i++) {
		lab[i] = layout_new(r, SIZE_BODY, 1, 0, label_w - 6);
		pango_layout_set_text(lab[i], labels[i], -1);
		val[i] = layout_new(r, SIZE_BODY, i == 0, 0, value_w);
		pango_layout_set_text(val[i], values[i], -1);
		int lh, vh;
		pango_layout_get_size(lab[i], NULL, &lh);
		pango_layout_get_size(val[i], NULL, &vh);
		rows[i] = fmax(lh, vh) / (double)PANGO_SCALE + 4;
		h += rows[i];
	}
	ensure(r, h);
	const double top = r->y;
	render_set_rgb(r->cr, r->st->panel, 1);
	cairo_rectangle(r->cr, MARGIN_X, top, CONTENT_W, h);
	cairo_fill(r->cr);
	render_set_rgb(r->cr, r->st->sub, 1);
	cairo_rectangle(r->cr, MARGIN_X, top, bar, h);
	cairo_fill(r->cr);
	double y = top + pad - 2;
	for(int i = 0; i < n; i++) {
		render_set_rgb(r->cr, r->st->title, 1);
		cairo_move_to(r->cr, MARGIN_X + bar + pad, y);
		pango_cairo_show_layout(r->cr, lab[i]);
		render_set_rgb(r->cr, r->st->ink, 1);
		cairo_move_to(r->cr, value_x, y);
		pango_cairo_show_layout(r->cr, val[i]);
		g_object_unref(lab[i]);
		g_object_unref(val[i]);
		y += rows[i];
	}
	r->y = top + h + 8;
}

static void report_header(struct report *r)
{
	const struct project_metadata *m = &r->d->project;
	info_panel(r);
	/* TRANSLATORS: under the header of the PDF report */
	muted(r, _("Measurement report made with a timegrapher. It is not a certificate of conformity with any standard."),
		1, 6);
	rule(r, 10);
	if(text_set(m->notes)) {
		/* TRANSLATORS: heading of the notes in the PDF report */
		heading(r, _("Notes"), SIZE_H2, 0, 2 * SIZE_BODY);
		markdown(r, m->notes);
	}
}

/* The positions with readings, as a mask for render_position_legend() */
static unsigned trend_positions(const struct trend *t)
{
	unsigned mask = 0;
	for(int i = 0; i < t->count; i++) {
		int p = t->samples[i].position;
		if(p >= POS_NONE && p < POS_COUNT && !t->samples[i].settling)
			mask |= 1u << p;
	}
	return mask ? mask : 1u << POS_NONE;
}

/* The Trends of a run: statistics, graphs and correlations */
static void report_trends(struct report *r, const struct trend *t)
{
	/* TRANSLATORS: heading of the PDF report: the long-term statistics and
	 * graphs of a run, as on the Trend page */
	heading(r, _("Trends"), SIZE_H2, 10, 60);
	char d[32];
	format_duration(d, sizeof(d), trend_elapsed(t));
	/* TRANSLATORS: PDF report: the time the trends of a run cover; %s is a
	 * duration such as 1:32:10 */
	char *interval = g_strdup_printf(_("Interval: the whole run, 0:00 to %s."), d);
	muted(r, interval, 0, 6);
	g_free(interval);
	if(!t->count) {
		muted(r, _("No readings were recorded"), 1, 6);
		return;
	}

	struct trend_render_options o = {
		.sections = TREND_RENDER_STATS,
		.span_s = 0,
		.rate_range = 0,
		.be_range = 0,
		.font = SIZE_GRAPH + .5,
		.calibrating = 0
	};
	double h = render_trend_stats_height(t, o.font);
	ensure(r, h);
	cairo_save(r->cr);
	cairo_translate(r->cr, MARGIN_X, r->y);
	render_trend(r->cr, CONTENT_W, h, t, &o, r->th);
	cairo_restore(r->cr);
	r->y += h + 6;

	o.sections = TREND_RENDER_GRAPHS;
	ensure(r, TREND_GRAPHS_HEIGHT);
	cairo_save(r->cr);
	cairo_translate(r->cr, MARGIN_X, r->y);
	render_trend(r->cr, CONTENT_W, TREND_GRAPHS_HEIGHT, t, &o, r->th);
	cairo_restore(r->cr);
	r->y += TREND_GRAPHS_HEIGHT + 10;

	// The correlations side by side, square
	static const char *titles[CORRELATION_COUNT] = {
		N_("Amplitude → rate"), N_("B.E. → rate"), N_("B.E. → amplitude")
	};
	const double gap = 12, side = (CONTENT_W - 2 * gap) / 3;
	heading(r, _("Correlations"), SIZE_H3, 8, side + 30);
	ensure(r, side + 30);
	for(int k = 0; k < CORRELATION_COUNT; k++) {
		double x = MARGIN_X + k * (side + gap);
		PangoLayout *l = layout_new(r, SIZE_SMALL, 1, 0, side);
		pango_layout_set_alignment(l, PANGO_ALIGN_CENTER);
		pango_layout_set_text(l, _(titles[k]), -1);
		render_set_rgb(r->cr, r->st->ink, 1);
		cairo_move_to(r->cr, x, r->y);
		pango_cairo_show_layout(r->cr, l);
		g_object_unref(l);

		struct correlation_options co = {
			.kind = k,
			.position = -1,
			.rate_range = 0,
			.be_range = 0,
			.font = 6.5
		};
		correlation_time_range(t, 0, &co.t0, &co.t1);
		cairo_save(r->cr);
		cairo_translate(r->cr, x, r->y + 12);
		render_correlation(r->cr, side, side, t, &co, r->th);
		cairo_restore(r->cr);
	}
	r->y += side + 12 + 4;
	render_position_legend(r->cr, r->th, MARGIN_X, r->y + 8, SIZE_GRAPH, trend_positions(t));
	r->y += 18;
}

/* RATE, AMP., B.E., BEAT, L.A. and cal of a reading, as on screen */
static void results_table(struct report *r, const struct snapshot *s)
{
	static const char *heads[6] = {"RATE", "AMP.", "B.E.", "BEAT", "L.A.", "cal"};
	char v[6][48];
	snprintf(v[0], sizeof(v[0]), "%+.1f s/d", s->rate);
	if(!s->rate_only && s->amp > 0)
		snprintf(v[1], sizeof(v[1]), "%.0f\xC2\xB0", s->amp);
	else
		snprintf(v[1], sizeof(v[1]), "\xE2\x80\x94");
	if(!s->rate_only)
		snprintf(v[2], sizeof(v[2]), "%.1f ms", s->be);
	else
		snprintf(v[2], sizeof(v[2]), "\xE2\x80\x94");
	if(s->bph)
		snprintf(v[3], sizeof(v[3]), "%d", s->bph);
	else
		snprintf(v[3], sizeof(v[3]), "%d (%s)", s->guessed_bph, _("guess"));
	snprintf(v[4], sizeof(v[4]), "%.0f\xC2\xB0", s->la);
	snprintf(v[5], sizeof(v[5]), "%+.1f s/d", s->cal / 10.0);

	// A table as in the manual: navy head row with white bold text, thin
	// grey borders, the values below
	const double head_h = 15, value_h = 22, inset = 5, col = CONTENT_W / 6;
	ensure(r, head_h + value_h + 8);
	const double top = r->y;
	render_set_rgb(r->cr, r->st->head_fill, 1);
	cairo_rectangle(r->cr, MARGIN_X, top, CONTENT_W, head_h);
	cairo_fill(r->cr);
	cairo_set_line_width(r->cr, .35);
	render_set_rgb(r->cr, r->st->line, 1);
	cairo_rectangle(r->cr, MARGIN_X, top, CONTENT_W, head_h + value_h);
	for(int i = 1; i < 6; i++) {
		cairo_move_to(r->cr, MARGIN_X + i * col, top);
		cairo_line_to(r->cr, MARGIN_X + i * col, top + head_h + value_h);
	}
	cairo_stroke(r->cr);
	for(int i = 0; i < 6; i++) {
		double x = MARGIN_X + i * col;
		PangoLayout *l = layout_new(r, SIZE_SMALL, 1, 0, col);
		pango_layout_set_alignment(l, PANGO_ALIGN_CENTER);
		pango_layout_set_text(l, heads[i], -1);
		render_set_rgb(r->cr, r->st->head_text, 1);
		cairo_move_to(r->cr, x, top + 3);
		pango_cairo_show_layout(r->cr, l);
		g_object_unref(l);
		l = layout_new(r, SIZE_H2, 1, 0, col);
		pango_layout_set_alignment(l, PANGO_ALIGN_CENTER);
		pango_layout_set_text(l, v[i], -1);
		render_set_rgb(r->cr, r->st->ink, 1);
		cairo_move_to(r->cr, x, top + head_h + inset - 1);
		pango_cairo_show_layout(r->cr, l);
		g_object_unref(l);
	}
	r->y = top + head_h + value_h + 8;
}

/* One diagnosis on its own page: readings, paper strip on the left and
 * waveforms on the right, as on screen, then its own notes */
static void report_measurement(struct report *r, const struct report_measurement *m,
	const char *subtitle, const char *notes, int own_page)
{
	const struct snapshot *s = m->snapshot;
	if(own_page && !r->blank)
		new_page(r);
	heading(r, m->title ? m->title : _("Snapshot"), own_page ? SIZE_H1 : SIZE_H2, 10,
		own_page ? 0 : 80);
	// When it was taken (local time), if the file knows it, and the position
	char *when = NULL;
	if(s->wall_time > 0) {
		GDateTime *t = g_date_time_new_from_unix_local(s->wall_time);
		if(t) {
			when = g_date_time_format(t, "%Y-%m-%d %H:%M:%S");
			g_date_time_unref(t);
		}
	}
	const char *position = watch_position_code(s->position) ? watch_position_label(s->position) : NULL;
	GString *line = g_string_new(NULL);
	char *from = NULL;
	if(s->saved_state && s->run_start > 0) {
		GDateTime *t = g_date_time_new_from_unix_local(s->run_start);
		if(t) {
			from = g_date_time_format(t, "%Y-%m-%d %H:%M:%S");
			g_date_time_unref(t);
		}
	}
	if(from && when)
		/* TRANSLATORS: PDF report: when a run started and ended (date and local time) */
		g_string_append_printf(line, _("From %s to %s"), from, when);
	else if(when)
		/* TRANSLATORS: PDF report: when a snapshot was taken (date and local time) */
		g_string_append_printf(line, _("Taken: %s"), when);
	g_free(from);
	if(position) {
		if(line->len) g_string_append(line, " \xC2\xB7 ");
		/* TRANSLATORS: PDF report: the watch position of a snapshot */
		g_string_append_printf(line, _("Position: %s"), position);
	}
	if(line->len)
		muted(r, line->str, 0, 2);
	g_string_free(line, TRUE);
	g_free(when);
	if(subtitle)
		muted(r, subtitle, 0, 2);
	r->y += 4;
	// The notes first, as for a run: they say what was being measured
	if(text_set(notes)) {
		heading(r, _("Notes"), SIZE_H2, 4, 2 * SIZE_BODY);
		markdown(r, notes);
		r->y += 4;
	}
	results_table(r, s);
	if(s->is_old)
		/* TRANSLATORS: PDF report, under the readings of a snapshot */
		muted(r, _("The reading was not current when this was taken: there was no valid signal."), 1, 6);

	const double block = MEASUREMENT_HEIGHT;
	ensure(r, block);
	const double strip_w = 190, gap = 14;
	const double wave_x = MARGIN_X + strip_w + gap, wave_w = CONTENT_W - strip_w - gap;
	const double legend_h = 16, wave_gap = 8;
	const double wave_h = (block - legend_h - 2 * wave_gap) / 3;

	// No rate guide in the report: it is an aid for the live display
	struct paperstrip_options po = {
		.tick_seconds = r->d->paperstrip_tick_seconds > 0 ? r->d->paperstrip_tick_seconds : 10,
		.window_seconds = r->d->paperstrip_window_seconds > 0 ? r->d->paperstrip_window_seconds : 60,
		.guide_rate = NAN,
		.now = s->timestamp,
		.font = SIZE_GRAPH,
		.legend = 1
	};
	cairo_save(r->cr);
	cairo_translate(r->cr, MARGIN_X, r->y);
	render_paperstrip(r->cr, strip_w, block, s, &po, r->th);
	cairo_restore(r->cr);

	for(int k = 0; k < 3; k++) {
		struct waveform_options wo = {
			.kind = k == 0 ? WAVEFORM_TIC : k == 1 ? WAVEFORM_TOC : WAVEFORM_PERIOD,
			.negative_span = m->negative_span,
			.font = SIZE_GRAPH
		};
		cairo_save(r->cr);
		cairo_translate(r->cr, wave_x, r->y + k * (wave_h + wave_gap));
		render_waveform(r->cr, wave_w, wave_h, s, &wo, r->th);
		cairo_restore(r->cr);
	}
	render_waveform_legend(r->cr, r->th, wave_x, r->y + block - 4, SIZE_GRAPH);
	r->y += block + 12;
}

/* ---- Runs ---- */

/* A date or time in local time (fmt as g_date_time_format()), NULL if
 * unknown. g_free(). */
static char *local_time(int64_t t, const char *fmt)
{
	if(t <= 0) return NULL;
	GDateTime *dt = g_date_time_new_from_unix_local(t);
	if(!dt) return NULL;
	char *s = g_date_time_format(dt, fmt);
	g_date_time_unref(dt);
	return s;
}

/* "2026-11-25 10:02–12:30", with both dates if it ended another day; NULL
 * if neither end is known. g_free(). */
static char *run_interval(const struct report_run *run)
{
	char *from_day = local_time(run->start, "%Y-%m-%d");
	char *to_day = local_time(run->end, "%Y-%m-%d");
	int same_day = from_day && to_day && !strcmp(from_day, to_day);
	char *from = local_time(run->start, "%Y-%m-%d %H:%M");
	char *to = local_time(run->end, same_day ? "%H:%M" : "%Y-%m-%d %H:%M");
	char *s = NULL;
	if(from && to)
		s = g_strdup_printf(same_day ? "%s\xE2\x80\x93%s" : "%s \xE2\x80\x93 %s", from, to);
	else if(from || to)
		s = g_strdup(from ? from : to);
	g_free(from_day);
	g_free(to_day);
	g_free(from);
	g_free(to);
	return s;
}

/* How long it lasted in seconds, -1 if unknown */
static double run_duration(const struct report_run *run)
{
	if(run->trend)
		return trend_elapsed(run->trend);
	if(run->start > 0 && run->end >= run->start)
		return (double)(run->end - run->start);
	return -1;
}

/* "Run 2", "Run 2 name", or "Run 3 (in progress)". g_free(). */
static char *run_name(const struct report_run *run)
{
	/* The same texts as the tabs and the notes panel */
	char *name = run->number > 0 ? g_strdup_printf(_("Run %d"), run->number) : g_strdup(_("Run"));
	if(text_set(run->name)) {
		char *t = g_strdup_printf("%s %s", name, run->name);
		g_free(name);
		name = t;
	}
	if(!run->in_progress) return name;
	char *s = g_strdup_printf(_("%s (in progress)"), name);
	g_free(name);
	return s;
}

/* Tables as in the manual: a navy head row with white bold text, thin grey
 * borders. Cells wrap; a table that goes on to another page repeats its head. */
#define TABLE_PAD 3

static PangoLayout *cell_layout(struct report *r, const char *text, double w, int head)
{
	PangoLayout *l = layout_new(r, SIZE_SMALL, head, 0, w - 2 * TABLE_PAD);
	pango_layout_set_alignment(l, PANGO_ALIGN_CENTER);
	pango_layout_set_text(l, text ? text : "", -1);
	return l;
}

static double row_height(struct report *r, int cols, const double *w, const char *const *text, int head)
{
	double h = 0;
	for(int c = 0; c < cols; c++) {
		PangoLayout *l = cell_layout(r, text[c], w[c], head);
		int lh;
		pango_layout_get_size(l, NULL, &lh);
		h = fmax(h, lh / (double)PANGO_SCALE);
		g_object_unref(l);
	}
	return h + 2 * TABLE_PAD;
}

/* head: the head row; sub: a row under another one (the snapshots of a
 * run), on a pale background */
static void draw_row(struct report *r, int cols, const double *w, const char *const *text,
	int head, int sub, double h)
{
	const double top = r->y;
	double total = 0;
	for(int c = 0; c < cols; c++)
		total += w[c];
	if(head || sub) {
		render_set_rgb(r->cr, head ? r->st->head_fill : r->st->panel, 1);
		cairo_rectangle(r->cr, MARGIN_X, top, total, h);
		cairo_fill(r->cr);
	}
	cairo_set_line_width(r->cr, .35);
	render_set_rgb(r->cr, r->st->line, 1);
	cairo_rectangle(r->cr, MARGIN_X, top, total, h);
	double x = MARGIN_X;
	for(int c = 0; c + 1 < cols; c++) {
		x += w[c];
		cairo_move_to(r->cr, x, top);
		cairo_line_to(r->cr, x, top + h);
	}
	cairo_stroke(r->cr);
	x = MARGIN_X;
	for(int c = 0; c < cols; c++) {
		PangoLayout *l = cell_layout(r, text[c], w[c], head);
		int lh;
		pango_layout_get_size(l, NULL, &lh);
		render_set_rgb(r->cr, head ? r->st->head_text : r->st->ink, 1);
		cairo_move_to(r->cr, x + TABLE_PAD, top + (h - lh / (double)PANGO_SCALE) / 2);
		pango_cairo_show_layout(r->cr, l);
		g_object_unref(l);
		x += w[c];
	}
	r->y = top + h;
}

/* cells: rows x cols texts, row after row; sub (NULL: none) marks the rows
 * that belong to the row above them */
static void table(struct report *r, int cols, const double *w, const char *const *heads,
	const char *const *cells, const int *sub, int rows)
{
	double head_h = row_height(r, cols, w, heads, 1);
	double first = rows ? row_height(r, cols, w, cells, 0) : 0;
	ensure(r, head_h + first);
	draw_row(r, cols, w, heads, 1, 0, head_h);
	for(int i = 0; i < rows; i++) {
		const char *const *row = cells + (size_t)i * cols;
		double h = row_height(r, cols, w, row, 0);
		if(r->y + h > BODY_BOTTOM) {
			new_page(r);
			r->blank = 0;
			draw_row(r, cols, w, heads, 1, 0, head_h);
		}
		draw_row(r, cols, w, row, 0, sub && sub[i], h);
	}
	r->y += 8;
}

/* First page: a line per run, with the means of its Trend, and under it a
 * line per snapshot of it in the report */
static void runs_summary(struct report *r)
{
	const struct report_data *d = r->d;
	/* TRANSLATORS: PDF report: heading of the table of the runs on the first page */
	heading(r, _("Runs"), SIZE_H2, 10, 50);
	enum { COLS = 8 };
	static const double share[COLS] = {.08, .25, .11, .13, .12, .10, .10, .11};
	double w[COLS];
	for(int c = 0; c < COLS; c++)
		w[c] = share[c] * CONTENT_W;
	const char *heads[COLS] = {
		/* TRANSLATORS: PDF report, heads of the table of the runs. Short */
		_("Run"), _("Period"), _("Duration"), _("Positions"),
		_("Mean rate"), _("Mean amp."), _("Mean B.E."), _("Snapshots")
	};
	// A row for each run, followed by a row for each of its snapshots in the
	// report, with its position, when it was taken and its readings
	int rows = 0;
	for(int i = 0; i < d->run_count; i++)
		rows += 1 + d->runs[i].snapshot_count;
	char **cells = g_new0(char *, (size_t)rows * COLS + 1);
	int *sub = g_new0(int, rows + 1);
	int any_in_progress = 0, row = 0;
	const char *none = "\xE2\x80\x94";
	for(int i = 0; i < d->run_count; i++) {
		const struct report_run *run = &d->runs[i];
		const struct trend *t = run->trend;
		char **c = cells + (size_t)row++ * COLS;
		// The run in progress is marked as on its tab
		c[0] = run->number > 0 ? g_strdup_printf(run->in_progress ? "%d \xE2\x97\x8F" : "%d", run->number)
			: g_strdup(none);
		any_in_progress |= run->in_progress;
		char *interval = run_interval(run);
		// The name given to the run, above its period
		if(text_set(run->name))
			c[1] = g_strdup_printf("%s\n%s", run->name, interval ? interval : none);
		else
			c[1] = g_strdup(interval ? interval : none);
		g_free(interval);
		double dur = run_duration(run);
		char text[32];
		if(dur >= 0) {
			format_duration(text, sizeof(text), dur);
			c[2] = g_strdup(text);
		} else
			c[2] = g_strdup("\xE2\x80\x94");
		GString *pos = g_string_new(NULL);
		for(int p = POS_NONE + 1; t && p < POS_COUNT; p++)
			if(t->pos_rate[p].n > 0) {
				if(pos->len) g_string_append_c(pos, ' ');
				g_string_append(pos, watch_position_code(p));
			}
		c[3] = g_string_free(pos, FALSE);
		if(!*c[3]) {
			g_free(c[3]);
			c[3] = g_strdup("\xE2\x80\x94");
		}
		c[4] = t && t->rate.n ? g_strdup_printf("%+.1f s/d", trend_stats_mean(&t->rate)) : g_strdup("\xE2\x80\x94");
		c[5] = t && t->amp.n ? g_strdup_printf("%.0f\xC2\xB0", trend_stats_mean(&t->amp)) : g_strdup("\xE2\x80\x94");
		c[6] = t && t->be.n ? g_strdup_printf("%.1f ms", trend_stats_mean(&t->be)) : g_strdup("\xE2\x80\x94");
		c[7] = g_strdup_printf("%d", run->snapshots_taken);

		for(int k = 0; k < run->snapshot_count; k++) {
			const struct snapshot *s = run->snapshots[k].snapshot;
			if(!s) continue;
			sub[row] = 1;
			c = cells + (size_t)row++ * COLS;
			c[0] = s->run > 0 && s->snap > 0 ? g_strdup_printf("%d.%d", s->run, s->snap)
				: g_strdup(run->snapshots[k].title ? run->snapshots[k].title : none);
			c[1] = local_time(s->wall_time, "%Y-%m-%d %H:%M:%S");
			if(!c[1]) c[1] = g_strdup(none);
			c[2] = g_strdup("");
			c[3] = g_strdup(watch_position_code(s->position) ? watch_position_code(s->position) : none);
			c[4] = g_strdup_printf("%+.1f s/d", s->rate);
			c[5] = !s->rate_only && s->amp > 0 ? g_strdup_printf("%.0f\xC2\xB0", s->amp) : g_strdup(none);
			c[6] = !s->rate_only ? g_strdup_printf("%.1f ms", s->be) : g_strdup(none);
			c[7] = g_strdup("");
		}
	}
	rows = row;	// snapshots without a measurement are not listed
	table(r, COLS, w, heads, (const char *const *)cells, sub, rows);
	g_free(sub);
	if(any_in_progress) {
		/* TRANSLATORS: PDF report, under the table of the runs, after a dot
		 * that marks a run in the table */
		char *note = g_strdup_printf("\xE2\x97\x8F %s", _("The run being recorded when the report was exported."));
		muted(r, note, 1, 4);
		g_free(note);
	}
	g_strfreev(cells);
}

/* A run: from a new page, its heading, notes and Trends, then its final
 * state and its snapshots, each on its own page */
static void report_run(struct report *r, const struct report_run *run)
{
	if(!r->blank)
		new_page(r);
	char *name = run_name(run);
	char *interval = run_interval(run);
	double dur = run_duration(run);
	GString *title = g_string_new(name);
	if(interval)
		g_string_append_printf(title, " \xC2\xB7 %s", interval);
	if(dur >= 0) {
		char text[32];
		format_duration(text, sizeof(text), dur);
		g_string_append_printf(title, " (%s)", text);
	}
	heading(r, title->str, SIZE_H1, 0, 0);
	g_string_free(title, TRUE);
	g_free(interval);
	g_free(name);

	// The run's notes are kept with its final state
	const char *notes = run->state.snapshot ? run->state.snapshot->notes : NULL;
	if(text_set(notes)) {
		heading(r, _("Notes"), SIZE_H2, 4, 2 * SIZE_BODY);
		markdown(r, notes);
	}
	if(run->show_trends) {
		if(run->trend)
			report_trends(r, run->trend);
		else {
			heading(r, _("Trends"), SIZE_H2, 10, 20);
			muted(r, _("No readings were recorded"), 1, 6);
		}
	}
	if(run->show_state && run->state.snapshot)
		report_measurement(r, &run->state, run->in_progress ?
			/* TRANSLATORS: PDF report, under the heading of the final state of the run being recorded */
			_("The real-time tab when the report was exported; the run goes on.") :
			/* TRANSLATORS: PDF report, under the heading of the final state of a run */
			_("End of the run: the real-time tab when it was paused, or when the session was saved."),
			NULL, 1);
	for(int k = 0; k < run->snapshot_count; k++) {
		const struct report_measurement *m = &run->snapshots[k];
		if(m->snapshot)
			report_measurement(r, m, NULL, m->snapshot->notes, 1);
	}
}

int report_render(cairo_t *cr, const struct report_data *d, int total_pages)
{
	struct report r = {
		.cr = cr,
		.d = d,
		.th = render_theme_get(d->theme),
		.st = d->theme == RENDER_DOC_MONO ? &style_mono : &style_colour,
		.y = MARGIN_TOP,
		.page = 1,
		.pages = total_pages,
		.blank = 1
	};
	// The text drawn by the graphs (Cairo's simple text) in the same family
	cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	header(&r);
	report_header(&r);
	if(d->summary && d->run_count)
		runs_summary(&r);
	for(int i = 0; i < d->run_count; i++)
		report_run(&r, &d->runs[i]);
	if(d->current.snapshot)
		/* TRANSLATORS: PDF report, under the heading of the current state */
		report_measurement(&r, &d->current,
			_("The real-time tab, frozen when the report was exported."), NULL, 1);
	int first = 1;
	for(int i = 0; i < d->other_count; i++) {
		const struct report_measurement *m = &d->others[i];
		if(!m->snapshot) continue;
		if(first) {
			if(!r.blank) new_page(&r);
			/* TRANSLATORS: PDF report: heading of the snapshots of older
			 * files, which do not say in which run they were taken */
			heading(&r, _("Other snapshots"), SIZE_H1, 0, 0);
			muted(&r, _("Snapshots of files written before runs: it is not known in which run they were taken."), 1, 4);
		}
		report_measurement(&r, m, NULL, m->snapshot->notes, !first);
		first = 0;
	}
	footer(&r);
	cairo_show_page(cr);
	return r.page;
}

static void measurement_free(struct report_measurement *m)
{
	if(m->snapshot)
		snapshot_destroy(m->snapshot);
	g_free(m->title);
	m->snapshot = NULL;
	m->title = NULL;
}

void report_data_free(struct report_data *d)
{
	project_metadata_free(&d->project);
	g_free(d->date);
	for(int i = 0; i < d->run_count; i++) {
		struct report_run *run = &d->runs[i];
		g_free(run->name);
		trend_free(run->trend);
		measurement_free(&run->state);
		for(int k = 0; k < run->snapshot_count; k++)
			measurement_free(&run->snapshots[k]);
		g_free(run->snapshots);
	}
	g_free(d->runs);
	measurement_free(&d->current);
	for(int i = 0; i < d->other_count; i++)
		measurement_free(&d->others[i]);
	g_free(d->others);
	if(d->logo)
		g_object_unref(d->logo);
	memset(d, 0, sizeof(*d));
}

/* ---- Writing ---- */

#ifdef CAIRO_HAS_PDF_SURFACE
static cairo_status_t write_to_file(void *closure, const unsigned char *data, unsigned int length)
{
	return fwrite(data, 1, length, closure) == length ?
		CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

static cairo_status_t discard(void *closure, const unsigned char *data, unsigned int length)
{
	UNUSED(closure);
	UNUSED(data);
	UNUSED(length);
	return CAIRO_STATUS_SUCCESS;
}

/* One complete PDF to the stream; 0 if Cairo reports no error */
static int render_pdf(cairo_write_func_t out, void *closure, const struct report_data *d,
	int total_pages, int *pages)
{
	cairo_surface_t *s = cairo_pdf_surface_create_for_stream(out, closure, PAGE_W, PAGE_H);
	if(cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(s);
		return 1;
	}
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 16, 0)
	const char *title = report_title(d);
	cairo_pdf_surface_set_metadata(s, CAIRO_PDF_METADATA_TITLE, title);
	cairo_pdf_surface_set_metadata(s, CAIRO_PDF_METADATA_CREATOR, PROGRAM_NAME " " VERSION);
#endif
	cairo_t *cr = cairo_create(s);
	int n = report_render(cr, d, total_pages);
	int failed = cairo_status(cr) != CAIRO_STATUS_SUCCESS;
	cairo_destroy(cr);
	cairo_surface_finish(s);
	if(cairo_surface_status(s) != CAIRO_STATUS_SUCCESS)
		failed = 1;
	cairo_surface_destroy(s);
	if(pages) *pages = n;
	return failed;
}

/* Two passes: the first counts the pages for "page N of M" */
static int pdf_writer(FILE *f, void *data)
{
	const struct report_data *d = data;
	int pages = 0;
	if(render_pdf(discard, NULL, d, 0, &pages))
		return 1;
	return render_pdf(write_to_file, f, d, pages, NULL);
}
#endif

int report_write_pdf(const char *path, const struct report_data *d)
{
#ifdef CAIRO_HAS_PDF_SURFACE
	return session_write_file_safely(path, pdf_writer, (void *)d) ? REPORT_ERROR_WRITE : 0;
#else
	UNUSED(path);
	UNUSED(d);
	return REPORT_ERROR_NO_PDF;
#endif
}
