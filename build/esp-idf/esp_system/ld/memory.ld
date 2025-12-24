/*

 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD

 *

 * SPDX-License-Identifier: Apache-2.0

 */
/* ESP32S2 Linker Script Memory Layout



   This file describes the memory layout (memory blocks) by virtual memory addresses.



   This linker script is passed through the C preprocessor to include configuration options.



   Please use preprocessor features sparingly!

   Restrict to simple macros with numeric values, and/or #if/#endif blocks.

*/
/*

 * Automatically generated file. DO NOT EDIT.

 * Espressif IoT Development Framework (ESP-IDF) 5.5.1 Configuration Header

 */
       
/* List of deprecated options */
/*

 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD

 *

 * SPDX-License-Identifier: Apache-2.0

 */
/* CPU instruction prefetch padding size for flash mmap scenario */
/*

 * PMP region granularity size

 * Software may determine the PMP granularity by writing zero to pmp0cfg, then writing all ones

 * to pmpaddr0, then reading back pmpaddr0. If G is the index of the least-significant bit set,

 * the PMP granularity is 2^G+2 bytes.

 */
/* CPU instruction prefetch padding size for memory protection scenario */
/* Memory alignment size for PMS */
    /* rtc timer data (s_rtc_timer_retain_mem, see esp_clk.c files). For rtc_timer_data_in_rtc_mem section. */
MEMORY
{
  /* All these values assume the flash cache is on, and have the blocks this uses subtracted from the length

  of the various regions. The 'data access port' dram/drom regions map to the same iram/irom regions but

  are connected to the data port of the CPU and eg allow bytewise access. */
  /* IRAM for CPU.*/
  iram0_0_seg (RX) : org = (0x40020000 + 0x2000 + 0x2000), len = 0x3FFDE000 - (0x3FFB0000 + 0x2000 + 0x2000)
  /* Even though the segment name is iram, it is actually mapped to flash

  */
  iram0_2_seg (RX) : org = 0x40080020, len = 0x780000-0x20
  /*

    (0x20 offset above is a convenience for the app binary image generation.

    Flash cache has 64KB pages. The .bin file which is flashed to the chip

    has a 0x18 byte file header, and each segment has a 0x08 byte segment

    header. Setting this offset makes it simple to meet the flash cache MMU's

    constraint that (paddr % 64KB == vaddr % 64KB).)

  */
  /* Shared data RAM, excluding memory reserved for bootloader and ROM bss/data/stack. */
  dram0_0_seg (RW) : org = (0x3FFB0000 + 0x2000 + 0x2000), len = 0x3FFDE000 - (0x3FFB0000 + 0x2000 + 0x2000)
  /* Flash mapped constant data */
  drom0_0_seg (R) : org = 0x3F000020, len = 0x3f0000-0x20
  /* (See iram0_2_seg for meaning of 0x20 offset in the above.) */
  /* RTC fast memory (executable). Persists over deep sleep.

   */
  rtc_iram_seg(RWX) : org = 0x40070000, len = 0x2000 - (0 + (24))
  /* RTC slow memory (data accessible). Persists over deep sleep.



     Start of RTC slow memory is reserved for ULP co-processor code + data, if enabled.

  */
  rtc_slow_seg(RW) : org = 0x50000000, len = 0x2000
  /* RTC fast memory (same block as above, rtc_iram_seg), viewed from data bus */
  rtc_data_seg(RW) : org = 0x3ff9e000, len = 0x2000 - (0 + (24))
  /* We reduced the size of rtc_data_seg and rtc_iram_seg by RESERVE_RTC_MEM value.

     It reserves the amount of RTC fast memory that we use for this memory segment.

     This segment is intended for keeping:

       - (lower addr) rtc timer data (s_rtc_timer_retain_mem, see esp_clk.c files).

       - (higher addr) bootloader rtc data (s_bootloader_retain_mem, when a Kconfig option is on).

     The aim of this is to keep data that will not be moved around and have a fixed address.

  */
  rtc_reserved_seg(RW) : org = 0x3ff9e000 + 0x2000 - (0 + (24)), len = (0 + (24))
  /* external memory, covers the dport, dram0, dram1 cacheable address space */
  extern_ram_seg(RWX) : org = 0x3F800000,
                                     len = 0x780000
}
_heap_start = _heap_low_start;
_heap_end = 0x40000000;
_data_seg_org = ORIGIN(rtc_data_seg);
/* The lines below define location alias for .rtc.data section based on Kconfig option.

   When the option is not defined then use slow memory segment

   else the data will be placed in fast memory segment */
REGION_ALIAS("rtc_data_location", rtc_slow_seg );
  REGION_ALIAS("default_code_seg", iram0_2_seg);
  REGION_ALIAS("default_rodata_seg", drom0_0_seg);
/**

 *  If rodata default segment is placed in `drom0_0_seg`, then flash's first rodata section must

 *  also be first in the segment.

 */
  ASSERT(_rodata_reserved_start == ORIGIN(default_rodata_seg),
         ".flash.appdesc section must be placed at the beginning of the rodata segment.")
