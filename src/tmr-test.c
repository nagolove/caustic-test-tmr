// vim: set colorcolumn=85
// vim: fdm=marker

#include "koh_tmr.h"
#include "munit.h"
#include <unistd.h>
#include <string.h>

static void sleep_sec(double s) {
	usleep((unsigned int)(s * 1000000.0));
}

// --- tmr_null ---

static MunitResult test_tmr_null(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	memset(&t, 0xFF, sizeof(t));
	tmr_null(&t);
	munit_assert_double(t.time_init, ==, 0.0);
	munit_assert_double(t.time_last, ==, 0.0);
	munit_assert_float(t.period, ==, 0.0f);
	munit_assert_float(t.time_amount, ==, 0.0f);
	munit_assert_float(t.time_loop, ==, 0.0f);
	munit_assert_float(t.time_start_delay, ==, 0.0f);
	munit_assert(!t.expired);
	munit_assert(!t.once);
	munit_assert(!t.is_inited);
	munit_assert(!t.paused);
	munit_assert(!t.use_period_range);
	return MUNIT_OK;
}

// --- tmr_init ---

static MunitResult test_tmr_init(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.1f;
	tmr_init(&t);
	munit_assert(t.is_inited);
	munit_assert(!t.expired);
	munit_assert_float(t.time_amount, ==, 0.0f);
	munit_assert_double(t.time_init, >, 0.0);
	return MUNIT_OK;
}

// --- tmr_once ---

static MunitResult test_tmr_once(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.once = true;
	tmr_init(&t);

	// ждём больше периода — должен сработать ровно один раз
	sleep_sec(0.06);

	bool fired = tmr_begin(&t);
	munit_assert(fired);
	munit_assert(t.expired);

	// повторно не срабатывает
	fired = tmr_begin(&t);
	munit_assert(!fired);

	return MUNIT_OK;
}

// --- tmr_periodic ---

static MunitResult test_tmr_periodic(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.03f;
	tmr_init(&t);

	// ждём период
	sleep_sec(0.05);

	bool fired = tmr_begin(&t);
	munit_assert(fired);
	munit_assert(!t.expired);
	tmr_end(&t);

	// сразу повторно не срабатывает
	fired = tmr_begin(&t);
	munit_assert(!fired);

	// ждём ещё период
	sleep_sec(0.05);

	fired = tmr_begin(&t);
	munit_assert(fired);

	return MUNIT_OK;
}

// --- tmr_time_loop ---

static MunitResult test_tmr_time_loop(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.time_loop = 0.08f;
	tmr_init(&t);

	// первый тик в середине time_loop
	sleep_sec(0.045);

	bool fired = tmr_begin(&t);
	munit_assert(fired);
	munit_assert_float(t.time_amount, >, 0.3f);
	munit_assert_float(t.time_amount, <, 0.9f);
	munit_assert(!t.expired);
	tmr_end(&t);

	// после окончания time_loop — expired
	sleep_sec(0.06);

	fired = tmr_begin(&t);
	munit_assert(!fired);
	munit_assert(t.expired);
	munit_assert_float(t.time_amount, ==, 1.0f);

	return MUNIT_OK;
}

// --- tmr_start_delay ---

static MunitResult test_tmr_start_delay(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.time_start_delay = 0.08f;
	tmr_init(&t);

	// до задержки — false
	bool fired = tmr_begin(&t);
	munit_assert(!fired);
	munit_assert_float(t.time_amount, ==, 0.0f);

	sleep_sec(0.04); // всего 0.04 < 0.08
	fired = tmr_begin(&t);
	munit_assert(!fired);

	// после задержки — true
	sleep_sec(0.06); // всего 0.10 > 0.08
	fired = tmr_begin(&t);
	munit_assert(fired);

	return MUNIT_OK;
}

// --- tmr_pause / tmr_resume ---

static MunitResult test_tmr_pause_resume(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	tmr_init(&t);

	// дожидаемся первого тика
	sleep_sec(0.04);
	bool fired = tmr_begin(&t);
	munit_assert(fired);
	tmr_end(&t);

	// ставим на паузу
	tmr_pause(&t);
	munit_assert(tmr_is_paused(&t));

	// пока на паузе — не срабатывает
	sleep_sec(0.1);
	fired = tmr_begin(&t);
	munit_assert(!fired);

	// возобновляем — замороженное время сдвинуто, сразу false
	tmr_resume(&t);
	munit_assert(!tmr_is_paused(&t));

	// после возобновления ждём период — должно сработать
	sleep_sec(0.04);
	fired = tmr_begin(&t);
	munit_assert(fired);

	return MUNIT_OK;
}

// --- tmr_set_paused ---

static MunitResult test_tmr_set_paused(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.1f;
	tmr_init(&t);

	tmr_set_paused(&t, true);
	munit_assert(tmr_is_paused(&t));

	tmr_set_paused(&t, false);
	munit_assert(!tmr_is_paused(&t));

	return MUNIT_OK;
}

// --- tmr_2str ---

static MunitResult test_tmr_2str(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.5f;
	t.once = true;
	tmr_init(&t);

	const char *s = tmr_2str(&t);
	munit_assert_not_null(s);
	munit_assert(strstr(s, "is_inited = true") != NULL);
	munit_assert(strstr(s, "expired = false") != NULL);
	munit_assert(strstr(s, "once = true") != NULL);
	munit_assert(strstr(s, "period = 0.5") != NULL);

	return MUNIT_OK;
}

// --- tmr_is_paused edge cases ---

static MunitResult test_tmr_is_paused_edge(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	// NULL
	munit_assert(!tmr_is_paused(NULL));

	// не инициализирован
	Tmr t;
	tmr_null(&t);
	munit_assert(!tmr_is_paused(&t));

	// инициализирован, но не на паузе
	t.period = 0.1f;
	tmr_init(&t);
	munit_assert(!tmr_is_paused(&t));

	return MUNIT_OK;
}

// --- tmr_begin с expired ---

static MunitResult test_tmr_begin_expired(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.1f;
	tmr_init(&t);
	t.expired = true;

	bool fired = tmr_begin(&t);
	munit_assert(!fired);

	return MUNIT_OK;
}

// --- tmr_once с time_loop: проверка time_amount ---

static MunitResult test_tmr_once_with_loop(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.once = true;
	t.time_loop = 0.1f;
	tmr_init(&t);

	sleep_sec(0.05); // 50% time_loop
	bool fired = tmr_begin(&t);
	munit_assert(fired);
	munit_assert(t.expired);
	munit_assert_float(t.time_amount, >, 0.3f);
	munit_assert_float(t.time_amount, <, 0.7f);

	return MUNIT_OK;
}

// --- tmr_end до start_delay: не сбивает time_last ---

static MunitResult test_tmr_end_before_start(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.time_start_delay = 0.1f;
	tmr_init(&t);

	double tl_before = t.time_last;
	tmr_end(&t);
	munit_assert_double(t.time_last, ==, tl_before);

	return MUNIT_OK;
}

// --- tmr_end после истечения ---

static MunitResult test_tmr_end_expired(const MunitParameter p[], void *data) {
	(void)p; (void)data;
	Tmr t;
	tmr_null(&t);
	t.period = 0.02f;
	t.once = true;
	tmr_init(&t);

	sleep_sec(0.05);
	tmr_begin(&t); // expired = true
	double tl = t.time_last;

	tmr_end(&t); // не должен менять time_last
	munit_assert_double(t.time_last, ==, tl);

	return MUNIT_OK;
}

// --- набор тестов ---

static MunitTest tmr_tests[] = {
	{ (char*) "/null",             test_tmr_null,             NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/init",             test_tmr_init,             NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/once",             test_tmr_once,             NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/periodic",         test_tmr_periodic,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/time_loop",        test_tmr_time_loop,        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/start_delay",      test_tmr_start_delay,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/pause_resume",     test_tmr_pause_resume,     NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/set_paused",       test_tmr_set_paused,       NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/2str",             test_tmr_2str,             NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/is_paused_edge",   test_tmr_is_paused_edge,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/begin_expired",    test_tmr_begin_expired,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/once_with_loop",   test_tmr_once_with_loop,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/end_before_start", test_tmr_end_before_start, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ (char*) "/end_expired",      test_tmr_end_expired,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite tmr_suite = {
	.prefix = (char*) "tmr",
	.tests = tmr_tests,
	.suites = NULL,
	.iterations = 1,
	.options = MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char **argv) {
	return munit_suite_main(&tmr_suite, (void*) "µnit", argc, argv);
}
