// SPDX-License-Identifier: GPL-2.0
#ifndef TEST_H
#define TEST_H
/*
 * Provides types and variables used when performing the memory tests.
 *
 * Copyright (C) 2020-2021 Martin Whitaker.
 */

#include <stdbool.h>
#include <stdint.h>

#include "pmem.h"

#include "barrier.h"
#include "spinlock.h"

/*
 * The maximum number of virtual CPUs supported. Note that the display can
 * only show the state of a maximum of 32 vCPUs.
 */
#define MAX_VCPUS   32

/*
 * The number of activated virtual CPUs.
 */
extern int num_vcpus;

/*
 * The current master virtual CPU.
 */
extern volatile int master_vcpu;

/*
 * A barrier used when running tests.
 */
extern barrier_t *run_barrier;

/*
 * A mutex used when reporting errors or printing trace information.
 */
extern spinlock_t *error_mutex;

/*
 * The word width (in bits) used for memory testing.
 */
#ifdef __x86_64__
#define TESTWORD_WIDTH  64
#else
#define TESTWORD_WIDTH  32
#endif

/*
 * The number of hex digits needed to display a memory test word.
 */
#define TESTWORD_DIGITS (TESTWORD_WIDTH / 4)

/*
 * The word type used for memory testing.
 */
typedef uintptr_t testword_t;

/*
 * A virtual memory segment descriptor.
 */
typedef struct {
    uintptr_t           pm_base_addr;
    testword_t          *start;
    testword_t          *end;
} vm_map_t;

/*
 * The list of memory segments currently mapped into virtual memory.
 */
extern volatile vm_map_t    vm_map[MAX_MEM_SEGMENTS];
/*
 * The number of memory segments currently mapped into virtual memory.
 */
extern volatile int         vm_map_size;

/*
 * The number of completed test passes.
 */
extern volatile int         pass_num;
/*
 * The current test number.
 */
extern volatile int         test_num;

/*
 * A flag indicating that testing should be restarted due to a configuration
 * change.
 */
extern volatile bool        restart;
/*
 * A flag indicating that the current test should be aborted.
 */
extern volatile bool        bail;

/*
 * The base address of the block of memory currently being tested.
 */
extern volatile uintptr_t   test_addr[MAX_VCPUS];

#endif // TEST_H
