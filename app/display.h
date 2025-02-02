// SPDX-License-Identifier: GPL-2.0
#ifndef DISPLAY_H
#define DISPLAY_H
/*
 * Provides (macro) functions that implement the UI display.
 * All knowledge about the display layout is encapsulated here.
 *
 * Copyright (C) 2020-2021 Martin Whitaker.
 */

#include <stdbool.h>

#include "screen.h"

#include "print.h"
#include "string.h"

#include "test.h"

#define ROW_MESSAGE_T   11
#define ROW_MESSAGE_B   (SCREEN_HEIGHT - 2)

#define ROW_SCROLL_T    (ROW_MESSAGE_T + 2)
#define ROW_SCROLL_B    (SCREEN_HEIGHT - 2)

#define ROW_FOOTER      (SCREEN_HEIGHT - 1)

#define BAR_LENGTH      40

#define ERROR_LIMIT     UINT64_C(999999999999)

#define display_cpu_model(str) \
    prints(0, 30, str)

#define display_cpu_clk(freq) \
    printf(1, 10, "%iMHz", freq)

#define display_cpu_addr_mode(str) \
    prints(1, 20, str)

#define display_l1_cache_size(size) \
    printf(2, 10, "%kB", (uintptr_t)(size))

#define display_l2_cache_size(size) \
    printf(3, 10, "%kB", (uintptr_t)(size))

#define display_l3_cache_size(size) \
    printf(4, 10, "%kB", (uintptr_t)(size))

#define display_memory_size(size) \
    printf(5, 10, "%kB", (uintptr_t)(size))

#define display_cpu_num(me) \
    printc(7, 7 + (me), '0' + ((me) % 10))

#define display_spinner(me, spin_state) \
    printc(8, 7 + (me), spin_state)

#define display_active_cpus(count) \
    printi(9, 7, count, 2, false, false)

#define display_total_cpus(count) \
    printi(9, 19, count, 2, false, false)

#define display_cpu_mode(str) \
    prints(9, 34, str)

#define display_pass_percentage(pct) \
    printi(1, 34, pct, 3, false, false)

#define display_pass_bar(length) \
    while (length > pass_bar_length) {          \
        printc(1, 39 + pass_bar_length, '#');   \
        pass_bar_length++;                      \
    }

#define display_test_percentage(pct) \
    printi(2, 34, pct, 3, false, false)

#define display_test_bar(length) \
    while (length > test_bar_length) {          \
        printc(2, 39 + test_bar_length, '#');   \
        test_bar_length++;                      \
    }

#define display_test_number(number) \
    printi(3, 36, number, 2, false, true)

#define display_test_description(str) \
    prints(3, 39, str)

#define display_test_addresses(pb, pe, total) \
    { \
        clear_screen_region(4, 39, 4, SCREEN_WIDTH - 1); \
        printf(4, 39, "%kB - %kB    %kB of %kB", pb, pe, (pe) - (pb), total); \
    }

#define display_test_stage_description(...) \
    { \
        clear_screen_region(4, 39, 4, SCREEN_WIDTH - 1); \
        printf(4, 39, __VA_ARGS__); \
    }

#define display_test_pattern_name(str) \
    { \
        clear_screen_region(5, 39, 5, SCREEN_WIDTH - 1); \
        prints(5, 39, str); \
    }

#define display_test_pattern_value(pattern) \
    { \
        clear_screen_region(5, 39, 5, SCREEN_WIDTH - 1); \
        printf(5, 39, "0x%0*x", TESTWORD_DIGITS, pattern); \
    }

#define display_test_pattern_values(pattern, offset) \
    { \
        clear_screen_region(5, 39, 5, SCREEN_WIDTH - 1); \
        printf(5, 39, "0x%0*x - %i", TESTWORD_DIGITS, pattern, offset); \
    }

#define display_run_time(hours, mins, secs) \
    printf(7, 47, "%i:%02i:%02i", hours, mins, secs)

#define display_temperature(temp) \
    printf(7, 71, "%2i%cC   ", temp, 0xf8)

#define display_pass_count(count) \
    printi(9, 47, count, 0, false, true)

#define display_error_count(count) \
    printi(9, 66, count, 0, false, true)

#define clear_message_area() \
    { \
        clear_screen_region(ROW_MESSAGE_T, 0, ROW_MESSAGE_B, SCREEN_WIDTH - 1); \
        scroll_message_row = ROW_SCROLL_T - 1; \
    }

#define display_pinned_message(row, col, ...) \
    printf(ROW_MESSAGE_T + row, col, __VA_ARGS__)

#define display_scrolled_message(col, ...) \
    printf(scroll_message_row, col, __VA_ARGS__)

#define display_notice(str) \
    prints(ROW_MESSAGE_T + 6, (SCREEN_WIDTH - strlen(str)) / 2, str)

#define clear_footer_message() \
    { \
        set_background_colour(WHITE); \
        clear_screen_region(ROW_FOOTER, 56, ROW_FOOTER, SCREEN_WIDTH - 1); \
        set_background_colour(BLUE);  \
    }

#define display_footer_message(str) \
    { \
        set_foreground_colour(BLUE);  \
        prints(ROW_FOOTER, 56, str);  \
        set_foreground_colour(WHITE); \
    }

#define trace(my_cpu, ...) \
    if (enable_trace) do_trace(my_cpu, __VA_ARGS__)

extern int scroll_message_row;

void display_init(void);

void display_start_run(void);

void display_start_pass(void);

void display_start_test(void);

void check_input(void);

void set_scroll_lock(bool enabled);

void toggle_scroll_lock(void);

void scroll(void);

void do_tick(int my_vcpu);

void do_trace(int my_vcpu, const char *fmt, ...);

#endif // DISPLAY_H
