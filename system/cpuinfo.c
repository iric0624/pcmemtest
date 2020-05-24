// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2020 Martin Whitaker.
//
// Derived from an extract of memtest86+ init.c:
//
// MemTest86+ V5 Specific code (GPL V2.0)
// By Samuel DEMEULEMEESTER, sdemeule@memtest.org
// http://www.canardpc.com - http://www.memtest.org
// ------------------------------------------------
// init.c - MemTest-86  Version 3.6
//
// Released under version 2 of the Gnu Public License.
// By Chris Brady

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpuid.h"
#include "io.h"
#include "tsc.h"

#include "cpuinfo.h"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

#define PIT_TICKS_50mS  59659    // PIT clock is 1.193182MHz

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

const char  *cpu_model = NULL;

uint32_t    imc_type = 0;

int         l1_cache = 0;
int         l2_cache = 0;
int         l3_cache = 0;

bool        no_temperature = false;

uint32_t    clks_per_msec = 0;

//------------------------------------------------------------------------------
// Private Functions
//------------------------------------------------------------------------------

static void determine_cache_size()
{
    switch (cpuid_info.vendor_id.str[0]) {
      case 'A':
        // AMD Processors (easy!)
        l1_cache = cpuid_info.cache_info.l1_d_size;
        l2_cache = cpuid_info.cache_info.l2_size;
        l3_cache = cpuid_info.cache_info.l3_size;
        l3_cache *= 512;
        break;
      case 'G':
        // Intel Processors
        l1_cache = 0;
        l2_cache = 0;
        l3_cache = 0;

        // Use CPUID(4) if it is available.
        if (cpuid_info.max_vcpuid > 3) {
            cpuid4_eax_t eax;
            cpuid4_ebx_t ebx;
            cpuid4_ecx_t ecx;
            uint32_t     dummy;

            // Loop through the cache leaves.
            int i = 0;
            do {
                cpuid(4, i, &eax.raw, &ebx.raw, &ecx.raw, &dummy);
                // Check for a valid cache type...
                if (eax.ctype == 1 || eax.ctype == 3) {
                    // Compute the cache size
                    int size = (ecx.number_of_sets + 1)
                             * (ebx.coherency_line_size + 1)
                             * (ebx.physical_line_partition + 1)
                             * (ebx.ways_of_associativity + 1);
                    size /= 1024;

                    switch (eax.level) {
                      case 1:
                        l1_cache += size;
                        break;
                      case 2:
                        l2_cache += size;
                        break;
                      case 3:
                        l3_cache += size;
                        break;
                      default:
                        break;
                    }
                }
                i++;
            } while (eax.ctype != 0);

            return;
        }

        // No CPUID(4) so we use the older CPUID(2) method.
        uint32_t v[4];
        uint8_t *dp = (uint8_t *)v;
        int i = 0;
        do {
            cpuid(2, 0, &v[0], &v[1], &v[2], &v[3]);

            // If bit 31 is set, this is an unknown format.
            for (int j = 0; j < 4; j++) {
                if (v[j] & (1 << 31)) {
                    v[j] = 0;
                }
            }

            // Byte 0 is level count, not a descriptor.
            for (int j = 1; j < 16; j++) {
                switch (dp[j]) {
                  case 0x6:
                  case 0xa:
                  case 0x66:
                    l1_cache += 8;
                    break;
                  case 0x8:
                  case 0xc:
                  case 0xd:
                  case 0x60:
                  case 0x67:
                    l1_cache += 16;
                    break;
                  case 0xe:
                    l1_cache += 24;
                    break;
                  case 0x9:
                  case 0x2c:
                  case 0x30:
                  case 0x68:
                    l1_cache += 32;
                    break;
                  case 0x39:
                  case 0x3b:
                  case 0x41:
                  case 0x79:
                    l2_cache += 128;
                    break;
                  case 0x3a:
                    l2_cache += 192;
                    break;
                  case 0x21:
                  case 0x3c:
                  case 0x3f:
                  case 0x42:
                  case 0x7a:
                  case 0x82:
                    l2_cache += 256;
                    break;
                  case 0x3d:
                    l2_cache += 384;
                    break;
                  case 0x3e:
                  case 0x43:
                  case 0x7b:
                  case 0x7f:
                  case 0x80:
                  case 0x83:
                  case 0x86:
                    l2_cache += 512;
                    break;
                  case 0x44:
                  case 0x78:
                  case 0x7c:
                  case 0x84:
                  case 0x87:
                    l2_cache += 1024;
                    break;
                  case 0x45:
                  case 0x7d:
                  case 0x85:
                    l2_cache += 2048;
                    break;
                  case 0x48:
                    l2_cache += 3072;
                    break;
                  case 0x4e:
                    l2_cache += 6144;
                    break;
                  case 0x23:
                  case 0xd0:
                    l3_cache += 512;
                    break;
                  case 0xd1:
                  case 0xd6:
                    l3_cache += 1024;
                    break;
                  case 0x25:
                  case 0xd2:
                  case 0xd7:
                  case 0xdc:
                  case 0xe2:
                    l3_cache += 2048;
                    break;
                  case 0x29:
                  case 0x46:
                  case 0x49:
                  case 0xd8:
                  case 0xdd:
                  case 0xe3:
                    l3_cache += 4096;
                    break;
                  case 0x4a:
                    l3_cache += 6144;
                    break;
                  case 0x47:
                  case 0x4b:
                  case 0xde:
                  case 0xe4:
                    l3_cache += 8192;
                    break;
                  case 0x4c:
                  case 0xea:
                    l3_cache += 12288;
                    break;
                  case 0x4d:
                    l3_cache += 16384;
                    break;
                  case 0xeb:
                    l3_cache += 18432;
                    break;
                  case 0xec:
                    l3_cache += 24576;
                    break;
                  default:
                    break;
                }
            }
        } while (++i < dp[0]);
        break;
      default:
        break;
    }
}

static void determine_imc(void)
{
    // Check AMD IMC
    if (cpuid_info.vendor_id.str[0] == 'A' && cpuid_info.version.family == 0xF)
    {
        switch (cpuid_info.version.extendedFamily)
        {
          case 0x0:
            imc_type = 0x0100; // Old K8
            break;
          case 0x1:
          case 0x2:
            imc_type = 0x0101; // K10 (Family 10h & 11h)
            break;
          case 0x3:
            imc_type = 0x0102; // A-Series APU (Family 12h)
            break;
          case 0x5:
            imc_type = 0x0103; // C- / E- / Z- Series APU (Family 14h)
            break;
          case 0x6:
            imc_type = 0x0104; // FX Series (Family 15h)
            break;
          case 0x7:
            imc_type = 0x0105; // Kabini & related (Family 16h)
            break;
          default:
            break;
        }
        return;
    }

    // Check Intel IMC
    if (cpuid_info.vendor_id.str[0] == 'G' && cpuid_info.version.family == 6 && cpuid_info.version.extendedModel)
    {
        switch (cpuid_info.version.model) {
          case 0x5:
            switch (cpuid_info.version.extendedModel) {
              case 2:
                imc_type = 0x0003;      // Core i3/i5 1st Gen 45 nm (NHM)
                break;
              case 3:
                no_temperature = true;  // Atom Clover Trail
                break;
              case 4:
                imc_type = 0x0007;      // HSW-ULT
                break;
              default:
                break;
            }
            break;
          case 0x6:
            if (cpuid_info.version.extendedModel == 3) {
                imc_type = 0x0009;      // Atom Cedar Trail
                no_temperature = true;
            }
            break;
          case 0x7:
            if (cpuid_info.version.extendedModel == 3) {
                imc_type = 0x000A;      // Atom Bay Trail
            }
            break;
          case 0xA:
            switch (cpuid_info.version.extendedModel) {
              case 0x1:
                imc_type = 0x0001;      // Core i7 1st Gen 45 nm (NHME)
                break;
              case 0x2:
                imc_type = 0x0004;      // Core 2nd Gen (SNB)
                break;
              case 0x3:
                imc_type = 0x0006;      // Core 3nd Gen (IVB)
                break;
              default:
                break;
            }
            break;
          case 0xC:
            switch (cpuid_info.version.extendedModel) {
              case 0x1:
                if (cpuid_info.version.stepping > 9) {
                    imc_type = 0x0008;  // Atom PineView
                }
                no_temperature = true;
                break;
              case 0x2:
                imc_type = 0x0002;      // Core i7 1st Gen 32 nm (WMR)
                break;
              case 0x3:
                imc_type = 0x0007;      // Core 4nd Gen (HSW)
                break;
              default:
                break;
            }
            break;
          case 0xD:
            imc_type = 0x0005;          // SNB-E
            break;
          case 0xE:
            imc_type = 0x0001;          // Core i7 1st Gen 45 nm (NHM)
            break;
          default:
            break;
        }
        return;
    }
}

static void determine_cpu_model(void)
{
    // If we can get a brand string use it, and we are done.
    if (cpuid_info.max_xcpuid >= 0x80000004) {
        cpu_model = cpuid_info.brand_id.str;
        determine_imc();
        return;
    }

    // The brand string is not available so we need to figure out CPU what we have.
    switch (cpuid_info.vendor_id.str[0]) {
      case 'A':
        // AMD Processors
        switch (cpuid_info.version.family) {
          case 4:
            switch (cpuid_info.version.model) {
              case 3:
                cpu_model = "AMD 486DX2";
                break;
              case 7:
                cpu_model = "AMD 486DX2-WB";
                break;
              case 8:
                cpu_model = "AMD 486DX4";
                break;
              case 9:
                cpu_model = "AMD 486DX4-WB";
                break;
              case 14:
                cpu_model = "AMD 5x86-WT";
                break;
              case 15:
                cpu_model = "AMD 5x86-WB";
                break;
              default:
                break;
            }
            break;
          case 5:
            switch (cpuid_info.version.model) {
              case 0:
              case 1:
              case 2:
              case 3:
                cpu_model = "AMD K5";
                l1_cache = 8;
                break;
              case 6:
              case 7:
                cpu_model = "AMD K6";
                break;
              case 8:
                cpu_model = "AMD K6-2";
                break;
              case 9:
                cpu_model = "AMD K6-III";
                break;
              case 13:
                cpu_model = "AMD K6-III+";
                break;
              default:
                break;
            }
            break;
          case 6:
            switch (cpuid_info.version.model) {
              case 1:
                cpu_model = "AMD Athlon (0.25)";
                break;
              case 2:
              case 4:
                cpu_model = "AMD Athlon (0.18)";
                break;
              case 6:
                if (l2_cache == 64) {
                    cpu_model = "AMD Duron (0.18)";
                } else {
                    cpu_model = "Athlon XP (0.18)";
                }
                break;
              case 8:
              case 10:
                if (l2_cache == 64) {
                    cpu_model = "AMD Duron (0.13)";
                } else {
                    cpu_model = "Athlon XP (0.13)";
                }
                break;
              case 3:
              case 7:
                cpu_model = "AMD Duron";
                // Duron stepping 0 CPUID for L2 is broken (AMD errata T13)
                if (cpuid_info.version.stepping == 0) {
                    // Hard code the right L2 size.
                    l2_cache = 64;
                }
                break;
              default:
                break;
            }
            break;
          default:
            // All AMD family values >= 10 have the Brand ID feature so we don't need to find the CPU type.
            break;
        }
        break;

      case 'G':
        // Transmeta Processors - vendor_id starts with "GenuineTMx86"
        if (cpuid_info.vendor_id.str[7] == 'T' ) {
            if (cpuid_info.version.family == 5) {
                cpu_model = "TM 5x00";
            } else if (cpuid_info.version.family == 15) {
                cpu_model = "TM 8x00";
            }
            l1_cache = cpuid_info.cache_info.l1_i_size + cpuid_info.cache_info.l1_d_size;
            l2_cache = cpuid_info.cache_info.l2_size;
            break;
        }
        // Intel Processors - vendor_id starts with "GenuineIntel"
        switch (cpuid_info.version.family) {
          case 4:
            switch (cpuid_info.version.model) {
              case 0:
              case 1:
                cpu_model = "Intel 486DX";
                break;
              case 2:
                cpu_model = "Intel 486SX";
                break;
              case 3:
                cpu_model = "Intel 486DX2";
                break;
              case 4:
                cpu_model = "Intel 486SL";
                break;
              case 5:
                cpu_model = "Intel 486SX2";
                break;
              case 7:
                cpu_model = "Intel 486DX2-WB";
                break;
              case 8:
                cpu_model = "Intel 486DX4";
                break;
              case 9:
                cpu_model = "Intel 486DX4-WB";
                break;
              default:
                break;
            }
            break;
          case 5:
            switch (cpuid_info.version.model) {
              case 0:
              case 1:
              case 2:
              case 3:
              case 7:
                cpu_model = "Pentium";
                if (l1_cache == 0) {
                    l1_cache = 8;
                }
                break;
              case 4:
              case 8:
                cpu_model = "Pentium-MMX";
                if (l1_cache == 0) {
                    l1_cache = 16;
                }
                break;
              default:
                break;
            }
            break;
          case 6:
            switch (cpuid_info.version.model) {
              case 0:
              case 1:
                cpu_model = "Pentium Pro";
                break;
              case 3:
              case 4:
                cpu_model = "Pentium II";
                break;
              case 5:
                if (l2_cache == 0) {
                    cpu_model = "Celeron";
                } else {
                    cpu_model = "Pentium II";
                }
                break;
              case 6:
                if (l2_cache == 128) {
                  cpu_model = "Celeron";
                } else {
                  cpu_model = "Pentium II";
                }
                break;
              case 7:
              case 8:
              case 11:
                if (l2_cache == 128) {
                    cpu_model = "Celeron";
                } else {
                    cpu_model = "Pentium III";
                }
                break;
              case 9:
                if (l2_cache == 512) {
                    cpu_model = "Celeron M (0.13)";
                } else {
                    cpu_model = "Pentium M (0.13)";
                }
                break;
              case 10:
                cpu_model = "Pentium III Xeon";
                break;
              case 12:
                l1_cache = 24;
                cpu_model = "Atom (0.045)";
                break;
              case 13:
                if (l2_cache == 1024) {
                    cpu_model = "Celeron M (0.09)";
                } else {
                    cpu_model = "Pentium M (0.09)";
                }
                break;
              case 14:
                cpu_model = "Intel Core";
                break;
              case 15:
                if (l2_cache == 1024) {
                    cpu_model = "Pentium E";
                } else {
                    cpu_model = "Intel Core 2";
                }
                break;
              default:
                break;
            }
            break;
          case 15:
            switch (cpuid_info.version.model) {
              case 0:
              case 1:
              case 2:
                if (l2_cache == 128) {
                    cpu_model = "Celeron";
                } else {
                    cpu_model = "Pentium 4";
                }
                break;
              case 3:
              case 4:
                if (l2_cache == 256) {
                    cpu_model = "Celeron (0.09)";
                } else {
                    cpu_model = "Pentium 4 (0.09)";
                }
                break;
              case 6:
                cpu_model = "Pentium D (65nm)";
                break;
              default:
                cpu_model = "Unknown Intel";
                break;
            }
            break;
          default:
            break;
        }
        break;

      case 'C':
        // VIA/Cyrix/Centaur Processors with CPUID
        if (cpuid_info.vendor_id.str[1] == 'e' ) {
            // CentaurHauls
            l1_cache = cpuid_info.cache_info.l1_i_size + cpuid_info.cache_info.l1_d_size;
            l2_cache = cpuid_info.cache_info.l2_size >> 8;
            switch (cpuid_info.version.family) {
              case 5:
                cpu_model = "Centaur 5x86";
                break;
              case 6: // VIA C3
                switch (cpuid_info.version.model) {
                  case 10:
                    cpu_model = "VIA C7 (C5J)";
                    l1_cache = 64;
                    l2_cache = 128;
                    break;
                  case 13:
                    cpu_model = "VIA C7 (C5R)";
                    l1_cache = 64;
                    l2_cache = 128;
                    break;
                  case 15:
                    cpu_model = "VIA Isaiah (CN)";
                    l1_cache = 64;
                    l2_cache = 128;
                    break;
                  default:
                    if (cpuid_info.version.stepping < 8) {
                        cpu_model = "VIA C3 Samuel2";
                    } else {
                        cpu_model = "VIA C3 Eden";
                    }
                  break;
                }
              default:
                break;
            }
        } else {                /* CyrixInstead */
            switch (cpuid_info.version.family) {
              case 5:
                switch (cpuid_info.version.model) {
                  case 0:
                    cpu_model = "Cyrix 6x86MX/MII";
                    break;
                  case 4:
                    cpu_model = "Cyrix GXm";
                    break;
                  default:
                    break;
                }
                break;
              case 6: // VIA C3
                switch (cpuid_info.version.model) {
                  case 6:
                    cpu_model = "Cyrix III";
                    break;
                  case 7:
                    if (cpuid_info.version.stepping < 8) {
                        cpu_model = "VIA C3 Samuel2";
                    } else {
                        cpu_model = "VIA C3 Ezra-T";
                    }
                    break;
                  case 8:
                    cpu_model = "VIA C3 Ezra-T";
                    break;
                  case 9:
                    cpu_model = "VIA C3 Nehemiah";
                    break;
                  default:
                    break;
                }
                // L1 = L2 = 64 KB from Cyrix III to Nehemiah
                l1_cache = 64;
                l2_cache = 64;
                break;
              default:
                break;
            }
        }
        break;
      default:
        // Unknown processor - make a guess at the family.
        switch (cpuid_info.version.family) {
          case 5:
            cpu_model = "586";
            break;
          case 6:
            cpu_model = "686";
            break;
          default:
            cpu_model = "Unidentified Processor";
            break;
        }
        break;
    }
}

static void measure_cpu_speed(void)
{
    if (cpuid_info.flags.rdtsc == 0) {
        return;
    }

    // Set up timer
    outb((inb(0x61) & ~0x02) | 0x01, 0x61);
    outb(0xb0, 0x43);
    outb(PIT_TICKS_50mS & 0xff, 0x42);
    outb(PIT_TICKS_50mS >> 8, 0x42);

    uint32_t start_time;
    rdtscl(start_time);

    int loops = 0;
    do {
        loops++;
    } while ((inb(0x61) & 0x20) == 0);

    uint32_t end_time;
    rdtscl(end_time);

    uint32_t run_time = end_time - start_time;

    // Make sure we have a credible result
    if (loops >= 4 && run_time >= 50000) {
       clks_per_msec = run_time / 50; 
    }
}

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void cpuinfo_init(void)
{
    // Get cache sizes for most AMD and Intel CPUs. Exceptions for old
    // CPUs are handled in determine_cpu_model().
    determine_cache_size();

    determine_cpu_model();

    measure_cpu_speed();
}
