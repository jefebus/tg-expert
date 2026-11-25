/* TG-28: project metadata and snapshot notes in .tgj files.
 *
 * Built like tests/rate_only.c: it includes computer.c and is linked with the
 * rest of the program. It only uses the public serializer API. */
#include <assert.h>

#include "../src/computer.c"

#define CHECK(X) do { if(!(X)) { fprintf(stderr, "line %d: %s\n", __LINE__, #X); abort(); } } while(0)

static struct snapshot *make_snapshot(const char *notes)
{
	struct snapshot *s = calloc(1, sizeof(*s));
	s->pb = calloc(1, sizeof(*s->pb));
	s->pb->waveform = calloc(100, sizeof(float));
	s->pb->sample_count = 100;
	s->pb->period = 50;
	s->pb->waveform_max = 1;
	s->pb->sample_rate = 48000;
	s->timestamp = 1;
	s->nominal_sr = 48000;
	s->bph = 21600;
	s->la = 52;
	s->cal = 0;
	s->guessed_bph = 21600;
	s->sample_rate = 48000;
	s->trace_zoom = 1;
	s->audio_status = AUDIO_STATUS_UNAVAILABLE;
	if(notes) {
		s->notes = strdup(notes);
		CHECK(s->notes);
	}
	return s;
}

struct loaded {
	struct snapshot **s;
	char **names;
	struct trend **trends;
	uint64_t count;
	struct project_metadata meta;
};

static int load(FILE *f, struct loaded *l, int with_meta)
{
	memset(l, 0, sizeof(*l));
	rewind(f);
	return read_file(f, &l->s, &l->names, &l->trends, &l->count, with_meta ? &l->meta : NULL);
}

static void loaded_free(struct loaded *l)
{
	for(uint64_t i = 0; i < l->count; i++) {
		snapshot_destroy(l->s[i]);
		free(l->names[i]);
		trend_free(l->trends[i]);
	}
	free(l->s);
	free(l->names);
	free(l->trends);
	project_metadata_free(&l->meta);
}

static int same(const char *a, const char *b)
{
	return strcmp(a ? a : "", b ? b : "") == 0;
}

/* The text of a file written with the given content */
static char *write_text(struct snapshot **s, char **names, uint64_t cnt,
	const struct project_metadata *meta, size_t *size)
{
	FILE *f = tmpfile();
	CHECK(f);
	CHECK(write_file(f, s, names, NULL, cnt, meta) == 0);
	long n = ftell(f);
	CHECK(n > 0);
	rewind(f);
	char *text = malloc(n + 1);
	CHECK(text && fread(text, 1, n, f) == (size_t)n);
	text[n] = 0;
	fclose(f);
	if(size) *size = n;
	return text;
}

static FILE *file_from(const char *text, size_t size)
{
	FILE *f = tmpfile();
	CHECK(f && fwrite(text, 1, size, f) == size);
	return f;
}

/* A file whose "data" struct holds `extra` before the snapshot list */
static FILE *file_with_extra(const char *extra, size_t extra_size)
{
	struct snapshot *s = make_snapshot(NULL);
	char *name = "n";
	size_t size;
	char *text = write_text(&s, &name, 1, NULL, &size);
	const char *marker = "Ldata;\nT;\n";
	char *at = strstr(text, marker);
	CHECK(at);
	at += strlen(marker);
	size_t head = at - text;
	char *all = malloc(size + extra_size);
	memcpy(all, text, head);
	memcpy(all + head, extra, extra_size);
	memcpy(all + head + extra_size, at, size - head);
	FILE *f = file_from(all, size + extra_size);
	free(all);
	free(text);
	snapshot_destroy(s);
	return f;
}

static FILE *file_with_extra_str(const char *extra)
{
	return file_with_extra(extra, strlen(extra));
}

/* One "project-metadata" block with the given body, e.g. "Ltitle;\nS1;a;\n" */
static char *block(const char *body)
{
	char *b = g_strdup_printf("Lproject-metadata;\nT;\n%sL__end__;\n", body);
	return b;
}

static void expect_rejected(const char *body)
{
	char *b = block(body);
	FILE *f = file_with_extra_str(b);
	struct loaded l;
	CHECK(load(f, &l, 1) != 0);
	CHECK(l.count == 0 && l.s == NULL);
	CHECK(project_metadata_is_empty(&l.meta));
	loaded_free(&l);
	fclose(f);
	g_free(b);
}

static struct project_metadata full_meta(void)
{
	struct project_metadata m;
	project_metadata_init(&m);
	m.title = strdup("Revisi\xC3\xB3n \"Omega\"; l\xC3\xADnea 1\nl\xC3\xADnea 2");
	m.project_reference = strdup("OT-2026/001;S5;x");
	m.watch_label = strdup("Reloj \xE2\x82\xAC \xF0\x9F\x95\x90");
	m.owner = strdup("Mar\xC3\xAD" "a Garc\xC3\xAD" "a");
	m.last_service_date = strdup("2024-02-29");
	m.notes = strdup("Notas\r\ncon 'comillas', \"dobles\", ; y L__end__;\n\n\xC3\xB1" "and\xC3\xBA");
	return m;
}

static void check_meta_equal(const struct project_metadata *a, const struct project_metadata *b)
{
	CHECK(same(a->title, b->title));
	CHECK(same(a->project_reference, b->project_reference));
	CHECK(same(a->watch_label, b->watch_label));
	CHECK(same(a->owner, b->owner));
	CHECK(same(a->last_service_date, b->last_service_date));
	CHECK(same(a->notes, b->notes));
}

static void test_round_trip(void)
{
	struct project_metadata m = full_meta();
	const char *n1 = "Notas del snapshot 1\n\xC3\xA1\xC3\xA9;\"'";
	struct snapshot *s[2] = {make_snapshot(n1), make_snapshot(NULL)};
	char *names[2] = {"Primero", "Segundo"};
	FILE *f = tmpfile();
	CHECK(f && write_file(f, s, names, NULL, 2, &m) == 0);

	struct loaded l;
	CHECK(load(f, &l, 1) == 0);
	CHECK(l.count == 2);
	check_meta_equal(&l.meta, &m);
	CHECK(l.s[0]->notes && !strcmp(l.s[0]->notes, n1));
	CHECK(l.s[1]->notes == NULL);
	CHECK(!strcmp(l.names[0], "Primero") && !strcmp(l.names[1], "Segundo"));
	loaded_free(&l);

	/* The metadata can be ignored by the caller */
	CHECK(load(f, &l, 0) == 0 && l.count == 2);
	loaded_free(&l);
	fclose(f);

	/* clone keeps notes in its own memory, destroy frees them */
	struct snapshot *c = snapshot_clone(s[0]);
	CHECK(c->notes && c->notes != s[0]->notes && !strcmp(c->notes, n1));
	s[0]->notes[0] = 'X';
	CHECK(c->notes[0] == 'N');
	snapshot_destroy(c);
	c = snapshot_clone(s[1]);
	CHECK(c->notes == NULL);
	snapshot_destroy(c);

	snapshot_destroy(s[0]);
	snapshot_destroy(s[1]);
	project_metadata_free(&m);
}

static void test_without_snapshots(void)
{
	struct project_metadata m;
	project_metadata_init(&m);
	m.title = strdup("Solo proyecto");
	FILE *f = tmpfile();
	CHECK(f && write_file(f, NULL, NULL, NULL, 0, &m) == 0);
	struct loaded l;
	CHECK(load(f, &l, 1) == 0);
	CHECK(l.count == 0);
	CHECK(same(l.meta.title, "Solo proyecto") && !l.meta.owner && !l.meta.notes);
	loaded_free(&l);
	fclose(f);
	project_metadata_free(&m);
}

static void test_old_and_empty(void)
{
	struct snapshot *s = make_snapshot(NULL);
	char *name = "viejo";
	size_t size;

	/* No metadata at all, like files from older versions */
	char *text = write_text(&s, &name, 1, NULL, &size);
	CHECK(!strstr(text, "project-metadata") && !strstr(text, "Lnotes;"));
	FILE *f = file_from(text, size);
	struct loaded l;
	CHECK(load(f, &l, 1) == 0 && l.count == 1);
	CHECK(project_metadata_is_empty(&l.meta));
	CHECK(l.s[0]->notes == NULL);
	loaded_free(&l);
	fclose(f);
	free(text);

	/* Empty metadata writes nothing, empty strings count as not set */
	struct project_metadata m;
	project_metadata_init(&m);
	m.title = strdup("");
	text = write_text(&s, &name, 1, &m, NULL);
	CHECK(!strstr(text, "project-metadata"));
	free(text);
	free(s->notes);
	s->notes = strdup("");
	text = write_text(&s, &name, 1, NULL, NULL);
	CHECK(!strstr(text, "Lnotes;"));
	free(text);
	project_metadata_free(&m);
	snapshot_destroy(s);
}

static void test_partial(void)
{
	struct project_metadata m;
	project_metadata_init(&m);
	m.watch_label = strdup("Solo etiqueta");
	m.last_service_date = strdup("2019");
	struct snapshot *s = make_snapshot(NULL);
	char *name = "x";
	FILE *f = tmpfile();
	CHECK(f && write_file(f, &s, &name, NULL, 1, &m) == 0);
	struct loaded l;
	CHECK(load(f, &l, 1) == 0);
	CHECK(!l.meta.title && !l.meta.project_reference && !l.meta.owner && !l.meta.notes);
	check_meta_equal(&l.meta, &m);
	loaded_free(&l);
	fclose(f);
	snapshot_destroy(s);
	project_metadata_free(&m);
}

static void test_dates(void)
{
	static const char *const good[] = {
		NULL, "", "2024", "1000", "9999", "2024-01", "2024-12", "2024-02-29",
		"2000-02-29", "2023-02-28", "2024-04-30", "2024-12-31"
	};
	static const char *const bad[] = {
		" ", "24", "999", "0999", "20245", "2024-", "2024-1", "2024-00", "2024-13",
		"2024-1-1", "2023-02-29", "1900-02-29", "2024-04-31", "2024-01-00",
		"2024-01-32", "2024/05/01", "2024-05-01T10:00", "2024-05-01Z", "abcd",
		" 2024", "2024 ", "2024-0a", "-024", "+2024", "2024-05-0\xC3\xB1"
	};
	for(size_t i = 0; i < sizeof(good) / sizeof(good[0]); i++)
		CHECK(project_date_valid(good[i]));
	for(size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
		CHECK(!project_date_valid(bad[i]));

	/* The precision typed is the precision stored */
	static const char *const keep[] = {"2024", "2024-03", "2024-03-09"};
	for(size_t i = 0; i < 3; i++) {
		struct project_metadata m;
		project_metadata_init(&m);
		m.last_service_date = strdup(keep[i]);
		FILE *f = tmpfile();
		CHECK(f && write_file(f, NULL, NULL, NULL, 0, &m) == 0);
		struct loaded l;
		CHECK(load(f, &l, 1) == 0);
		CHECK(l.meta.last_service_date && !strcmp(l.meta.last_service_date, keep[i]));
		loaded_free(&l);
		fclose(f);
		project_metadata_free(&m);
	}

	/* An invalid date is refused when writing and when reading */
	struct project_metadata m;
	project_metadata_init(&m);
	m.last_service_date = strdup("2023-02-29");
	FILE *f = tmpfile();
	CHECK(f && write_file(f, NULL, NULL, NULL, 0, &m) != 0);
	fclose(f);
	project_metadata_free(&m);
	expect_rejected("Llast-service-date;\nS10;2023-02-29;\n");
	expect_rejected("Llast-service-date;\nS5;2024-;\n");
}

static void test_limits(void)
{
	struct project_metadata m;
	char *big = malloc(PROJECT_NOTES_MAX + 2);
	CHECK(big);

	/* Exactly at the limit round-trips */
	project_metadata_init(&m);
	memset(big, 'a', PROJECT_FIELD_MAX);
	big[PROJECT_FIELD_MAX] = 0;
	m.title = strdup(big);
	m.project_reference = strdup(big);
	m.watch_label = strdup(big);
	m.owner = strdup(big);
	memset(big, 'n', PROJECT_NOTES_MAX);
	big[PROJECT_NOTES_MAX] = 0;
	m.notes = strdup(big);
	struct snapshot *s = make_snapshot(big);
	char *name = "n";
	FILE *f = tmpfile();
	CHECK(f && write_file(f, &s, &name, NULL, 1, &m) == 0);
	struct loaded l;
	CHECK(load(f, &l, 1) == 0 && l.count == 1);
	check_meta_equal(&l.meta, &m);
	CHECK(l.s[0]->notes && strlen(l.s[0]->notes) == PROJECT_NOTES_MAX);
	loaded_free(&l);
	fclose(f);

	/* One byte over is refused when writing, and nothing is truncated */
	char **fields[] = {&m.title, &m.project_reference, &m.watch_label, &m.owner};
	for(int i = 0; i < 4; i++) {
		char *keep = *fields[i];
		char *longer = malloc(PROJECT_FIELD_MAX + 2);
		memset(longer, 'a', PROJECT_FIELD_MAX + 1);
		longer[PROJECT_FIELD_MAX + 1] = 0;
		*fields[i] = longer;
		f = tmpfile();
		CHECK(f && write_file(f, &s, &name, NULL, 1, &m) != 0);
		fclose(f);
		*fields[i] = keep;
		free(longer);
	}
	char *keep = m.notes;
	memset(big, 'n', PROJECT_NOTES_MAX + 1);
	big[PROJECT_NOTES_MAX + 1] = 0;
	m.notes = big;
	f = tmpfile();
	CHECK(f && write_file(f, &s, &name, NULL, 1, &m) != 0);
	fclose(f);
	m.notes = keep;
	free(s->notes);
	s->notes = strdup(big);
	f = tmpfile();
	CHECK(f && write_file(f, &s, &name, NULL, 1, NULL) != 0);
	fclose(f);
	snapshot_destroy(s);
	project_metadata_free(&m);

	/* ... and when reading */
	char *longf = g_strnfill(PROJECT_FIELD_MAX + 1, 'a');
	char *body = g_strdup_printf("Ltitle;\nS%d;%s;\n", PROJECT_FIELD_MAX + 1, longf);
	g_free(longf);
	expect_rejected(body);
	g_free(body);
	body = g_strdup_printf("Lnotes;\nS%d;%s;\n", PROJECT_NOTES_MAX + 1, big);
	expect_rejected(body);
	g_free(body);
	/* A huge declared length must fail without allocating it */
	expect_rejected("Lnotes;\nS18446744073709551615;abc;\n");
	expect_rejected("Lnotes;\nS100000;abc;\n");
	free(big);
}

static void test_text_rules(void)
{
	/* UTF-8 only, and no NUL bytes inside */
	expect_rejected("Ltitle;\nS3;a\xFF" "b;\n");
	expect_rejected("Ltitle;\nS2;\xC3;\n");
	expect_rejected("Ltitle;\nS2;\xC0\xAF;\n");
	char with_nul[] = "Lproject-metadata;\nT;\nLtitle;\nS3;a\0b;\nL__end__;\n";
	FILE *f = file_with_extra(with_nul, sizeof(with_nul) - 1);
	struct loaded l;
	CHECK(load(f, &l, 1) != 0 && l.count == 0 && project_metadata_is_empty(&l.meta));
	loaded_free(&l);
	fclose(f);

	struct project_metadata m;
	project_metadata_init(&m);
	m.title = strdup("a\xFF" "b");
	f = tmpfile();
	CHECK(f && write_file(f, NULL, NULL, NULL, 0, &m) != 0);
	fclose(f);
	project_metadata_free(&m);

	/* The length must be exact */
	expect_rejected("Ltitle;\nS5;abc;\n");
	expect_rejected("Ltitle;\nS2;abc;\n");
	expect_rejected("Ltitle;\nI1;\n");
	expect_rejected("Ltitle;\nS3;abc");
}

static void test_structure(void)
{
	struct loaded l;
	FILE *f;

	/* Duplicates */
	expect_rejected("Ltitle;\nS1;a;\nLtitle;\nS1;b;\n");
	expect_rejected("Lnotes;\nS0;;\nLnotes;\nS1;b;\n");
	expect_rejected("Lschema-version;\nI1;\nLschema-version;\nI1;\n");
	char *b = block("Ltitle;\nS1;a;\n");
	char *twice = g_strconcat(b, b, NULL);
	f = file_with_extra_str(twice);
	CHECK(load(f, &l, 1) != 0 && l.count == 0 && project_metadata_is_empty(&l.meta));
	loaded_free(&l);
	fclose(f);
	g_free(twice);
	g_free(b);

	/* Bad schema version, or a block that is not a struct */
	expect_rejected("Lschema-version;\nI0;\n");
	expect_rejected("Lschema-version;\nS1;1;\n");
	f = file_with_extra_str("Lproject-metadata;\nS1;a;\n");
	CHECK(load(f, &l, 1) != 0);
	loaded_free(&l);
	fclose(f);

	/* Unknown fields, of any kind, and a newer schema are accepted and skipped */
	b = block("Lschema-version;\nI2;\nLfuture-text;\nS3;x;y;\nLfuture-number;\nI7;\n"
		"Lfuture-list;\nA2;\nI1;\nS1;z;\nLfuture-struct;\nT;\nLa;\nI1;\nL__end__;\n"
		"Ltitle;\nS5;Hola!;\nLowner;\nS3;Ana;\n");
	f = file_with_extra_str(b);
	CHECK(load(f, &l, 1) == 0 && l.count == 1);
	CHECK(same(l.meta.title, "Hola!") && same(l.meta.owner, "Ana"));
	CHECK(!l.meta.notes && !l.meta.watch_label);
	loaded_free(&l);
	fclose(f);
	g_free(b);

	/* Unknown top-level fields next to the metadata */
	f = file_with_extra_str("Lother-block;\nT;\nLx;\nI1;\nL__end__;\n");
	CHECK(load(f, &l, 1) == 0 && l.count == 1 && project_metadata_is_empty(&l.meta));
	loaded_free(&l);
	fclose(f);

	/* The block may follow the snapshot list too */
	struct snapshot *s = make_snapshot(NULL);
	char *name = "n";
	size_t size;
	char *text = write_text(&s, &name, 1, NULL, &size);
	char *end = strstr(text, "L__end__;\n");
	CHECK(end);
	char *last = end;
	for(char *p = end; (p = strstr(p + 1, "L__end__;\n")); ) last = p;
	b = block("Ltitle;\nS5;Final;\n");
	char *all = g_strdup_printf("%.*s%s%s", (int)(last - text), text, b, last);
	f = file_from(all, strlen(all));
	CHECK(load(f, &l, 1) == 0 && l.count == 1 && same(l.meta.title, "Final"));
	loaded_free(&l);
	fclose(f);
	g_free(all);
	g_free(b);

	/* Anything after "data" is still refused, and nothing is left behind */
	char *stray = g_strconcat(text, "Lextra;\nI1;\n", NULL);
	f = file_from(stray, strlen(stray));
	CHECK(load(f, &l, 1) != 0 && l.count == 0 && l.s == NULL);
	CHECK(project_metadata_is_empty(&l.meta));
	loaded_free(&l);
	fclose(f);
	g_free(stray);
	free(text);
	snapshot_destroy(s);
}

static void test_snapshot_notes_rules(void)
{
	struct snapshot *s = make_snapshot(NULL);
	char *name = "n";
	size_t size;
	char *text = write_text(&s, &name, 1, NULL, &size);
	snapshot_destroy(s);
	const char *marker = "Lpb->waveform;\n";
	char *at = strstr(text, marker);
	CHECK(at);
	size_t head = at - text;

	static const struct {
		const char *extra;
		int ok;
	} cases[] = {
		{"Lnotes;\nS2;hi;\n", 1},
		{"Lnotes;\nS0;;\n", 1},
		{"Lnotes;\nS2;hi;\nLnotes;\nS2;ho;\n", 0},
		{"Lnotes;\nS2;h\xFF;\n", 0},
		{"Lnotes;\nI3;\n", 0},
		{"Lnotes;\nS9;hi;\n", 0},
	};
	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char *all = g_strdup_printf("%.*s%s%s", (int)head, text, cases[i].extra, at);
		FILE *f = file_from(all, strlen(all));
		struct loaded l;
		int r = load(f, &l, 1);
		CHECK((r == 0) == cases[i].ok);
		if(r) CHECK(l.count == 0 && l.s == NULL);
		if(!r && i == 0) CHECK(!strcmp(l.s[0]->notes, "hi"));
		if(!r && i == 1) CHECK(l.s[0]->notes == NULL);
		loaded_free(&l);
		fclose(f);
		g_free(all);
	}
	free(text);
}

/* TG-30/TG-34: a run is marked with an optional role, its number and its start */
static void test_saved_state_role(void)
{
	struct snapshot *s[2] = {make_snapshot(NULL), make_snapshot(NULL)};
	s[1]->saved_state = 1;
	s[1]->run = 2;
	s[1]->run_start = 1795620000;
	s[0]->run = 2;	// the snapshot was taken in run 2
	s[0]->snap = 3;	// its third snapshot: "2.3"
	s[1]->snap = 5;	// not for a run: not written
	char *names[2] = {"Captura", "Estado al guardar"};
	s[0]->wall_time = 1795628400;	// when it was taken
	size_t size;
	char *text = write_text(s, names, 2, NULL, &size);
	/* The wall-clock time is written only when known, and read back */
	char *wt = strstr(text, "Lwall_time;\nI1795628400;\n");
	CHECK(wt && !strstr(wt + 1, "Lwall_time;"));
	/* Written only for the run, so plain snapshots look as before */
	char *first = strstr(text, "Lrole;\nS3;run;\n");
	CHECK(first && !strstr(first + 1, "Lrole;"));
	char *start = strstr(text, "Lrun_start;\nI1795620000;\n");
	CHECK(start && !strstr(start + 1, "Lrun_start;"));
	/* A run's name: the whole title for older programs, the user's name apart */
	CHECK(strstr(text, "Lname;\nS23;Run 2 Estado al guardar;\n"));
	CHECK(strstr(text, "Lrun_name;\nS17;Estado al guardar;\n"));
	char *snap = strstr(text, "Lsnap;\nI3;\n");
	CHECK(snap && !strstr(snap + 1, "Lsnap;"));
	FILE *f = file_from(text, size);
	struct loaded l;
	CHECK(load(f, &l, 1) == 0 && l.count == 2);
	CHECK(!l.s[0]->saved_state && l.s[1]->saved_state);
	CHECK(l.s[0]->snap == 3 && l.s[1]->snap == 0);
	CHECK(l.s[0]->wall_time == 1795628400 && l.s[1]->wall_time == 0);
	CHECK(l.s[0]->run == 2 && l.s[1]->run == 2);
	CHECK(l.s[0]->run_start == 0 && l.s[1]->run_start == 1795620000);
	/* The name is not what tells them apart */
	CHECK(!strcmp(l.names[1], "Estado al guardar"));
	/* A copy keeps the role */
	struct snapshot *c = snapshot_clone(l.s[1]);
	CHECK(c->saved_state);
	snapshot_destroy(c);
	loaded_free(&l);
	fclose(f);

	/* Unknown roles from newer versions load as plain snapshots, a repeated
	 * or malformed role is an error */
	char *at = strstr(text, "Lrole;\nS3;run;\n");
	size_t head = at - text, skip = strlen("Lrole;\nS3;run;\n");
	static const struct {
		const char *role;
		int ok, saved;
	} cases[] = {
		{"Lrole;\nS6;future;\n", 1, 0},
		{"Lrole;\nS0;;\n", 1, 0},
		// written by 0.11.x before runs: read as a run
		{"Lrole;\nS11;saved-state;\n", 1, 1},
		{"Lrole;\nS3;run;\nLrole;\nS3;run;\n", 0, 0},
		{"Lrole;\nI1;\n", 0, 0},
		{"", 1, 0},
	};
	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char *all = g_strdup_printf("%.*s%s%s", (int)head, text, cases[i].role, at + skip);
		f = file_from(all, strlen(all));
		int r = load(f, &l, 1);
		CHECK((r == 0) == cases[i].ok);
		if(!r) CHECK(l.count == 2 && l.s[1]->saved_state == cases[i].saved);
		loaded_free(&l);
		fclose(f);
		g_free(all);
	}
	free(text);
	snapshot_destroy(s[0]);
	snapshot_destroy(s[1]);
}

static void test_helpers(void)
{
	struct project_metadata a, b;
	project_metadata_init(&a);
	project_metadata_init(&b);
	CHECK(project_metadata_is_empty(&a));
	a = full_meta();
	CHECK(!project_metadata_is_empty(&a));
	b.title = strdup("sera sustituido");
	CHECK(project_metadata_copy(&b, &a) == 0);
	check_meta_equal(&a, &b);
	CHECK(a.title != b.title && a.notes != b.notes);
	free(a.title);
	a.title = strdup("otro");
	CHECK(!same(a.title, b.title));
	project_metadata_free(&a);
	CHECK(project_metadata_is_empty(&a) && !a.title);
	project_metadata_free(&b);
	project_metadata_free(&b);

	CHECK(project_text_valid(NULL, 10) && project_text_valid("", 0));
	CHECK(project_text_valid("\xC3\xA1\xC3\xA1", 4) && !project_text_valid("\xC3\xA1\xC3\xA1", 3));
	CHECK(!project_text_valid("\xFF", 10));
}

int main(void)
{
	test_round_trip();
	test_without_snapshots();
	test_old_and_empty();
	test_partial();
	test_dates();
	test_limits();
	test_text_rules();
	test_structure();
	test_snapshot_notes_rules();
	test_saved_state_role();
	test_helpers();
	return 0;
}
