/*
 * File      : cache.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2011, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-07-09     Bernard      first version
 * 2011-08-08     lgnq         modified for LS1B
 */

#include "../common/mipsregs.h"

#define K0BASE            0x80000000
#define PRID_LS1B        0x4220

extern void Clear_TagLo (void);
extern void Invalidate_Icache_Ls1b(unsigned int);
extern void Invalidate_Dcache_ClearTag_Ls1b(unsigned int);
extern void Invalidate_Dcache_Fill_Ls1b(unsigned int);
extern void Writeback_Invalidate_Dcache(unsigned int);

typedef struct cacheinfo_t
{
    unsigned int    icache_size;
    unsigned int    dcache_size;
    unsigned int    icacheline_size;
    unsigned int    dcacheline_size;
} cacheinfo_t ;

typedef struct cacheop_t
{
    void (*Clear_TagLo) (void);
    void (*Invalidate_Icache) (unsigned int);
    void (*Invalidate_Dcache_Fill) (unsigned int);
    void (*Invalidate_Dcache_ClearTag) (unsigned int);
    void (*Init_Cache)(void);
} cacheop_t ;

static cacheop_t cacheop, *pcacheop;
static cacheinfo_t cacheinfo, *pcacheinfo;

int identify_cpu(void)
{
    unsigned int os_arch_id;

    pcacheop = &cacheop;
    pcacheinfo = &cacheinfo;

    printk("CPU configure: 0x%08x\n", read_c0_config());
    os_arch_id = read_c0_prid();
    switch (os_arch_id) {
    case PRID_LS1B:
        printk("CPU:LS1B\n");
        pcacheop->Clear_TagLo = Clear_TagLo;
        pcacheop->Invalidate_Icache = Invalidate_Icache_Ls1b;
        pcacheop->Invalidate_Dcache_Fill = Invalidate_Dcache_Fill_Ls1b;
        pcacheop->Invalidate_Dcache_ClearTag = Invalidate_Dcache_ClearTag_Ls1b;
        break;
    default:
        printk("Unknown CPU type, system halted!\n");
        while (1) {
            ;
        }
        break;
    }

    return 0;
}

void probe_cache(void)
{
    unsigned int config1 = read_c0_config1();
    unsigned int icache_size, icache_line_size, icache_sets, icache_ways;
    unsigned int dcache_size, dcache_line_size, dcache_sets, dcache_ways;

    if ((icache_line_size = ((config1 >> 19) & 7)))
        icache_line_size = 2 << icache_line_size;
    else
        icache_line_size = icache_line_size;
    icache_sets = 64 << ((config1 >> 22) & 7);
    icache_ways = 1 + ((config1 >> 16) & 7);
    icache_size = icache_sets * icache_ways * icache_line_size;

    if ((dcache_line_size = ((config1 >> 10) & 7)))
        dcache_line_size = 2 << dcache_line_size;
    else
        dcache_line_size = dcache_line_size;
    dcache_sets = 64 << ((config1 >> 13) & 7);
    dcache_ways = 1 + ((config1 >> 7) & 7);
    dcache_size = dcache_sets * dcache_ways * dcache_line_size;

    printk("DCache %2dkb, linesize %d bytes.\n", dcache_size >> 10, dcache_line_size);
    printk("ICache %2dkb, linesize %d bytes.\n", icache_size >> 10, icache_line_size);

    pcacheinfo->icache_size = icache_size;
    pcacheinfo->dcache_size = dcache_size;
    pcacheinfo->icacheline_size = icache_line_size;
    pcacheinfo->dcacheline_size = dcache_line_size;

    return ;
}

void invalidate_writeback_dcache_all(void)
{
    unsigned int start = K0BASE;
    unsigned int end = (start + pcacheinfo->dcache_size);

    while (start < end) {
        Writeback_Invalidate_Dcache(start);  //hit writeback invalidate
        start += pcacheinfo->dcacheline_size;
    }
}

void invalidate_writeback_dcache(uint32_t addr, int size)
{
    uint32_t start, end;

    start = (addr +pcacheinfo->dcacheline_size -1) & (- pcacheinfo->dcacheline_size);
    end = (end + size + pcacheinfo->dcacheline_size -1) & ( -pcacheinfo->dcacheline_size);

    while (start <end) {
        Writeback_Invalidate_Dcache(start);
        start += pcacheinfo->dcacheline_size;
    }
}

void invalidate_icache_all(void)
{
    unsigned int start = K0BASE;
    unsigned int end = (start + pcacheinfo->icache_size);

    while (start < end) {
        pcacheop->Invalidate_Icache(start);
        start += pcacheinfo->icacheline_size;
    }
}

void invalidate_dcache_all(void)
{
    unsigned int start = K0BASE;
    unsigned int end  = (start + pcacheinfo->dcache_size);
    while (start <end) {
        Invalidate_Dcache_Fill_Gc3210I(start);
        start += pcacheinfo->icacheline_size;
    }
}

//with cache disabled
void init_dcache(void)
{
    unsigned int start = K0BASE;
    unsigned int end = (start + pcacheinfo->dcache_size);

    while (start < end) {
        pcacheop->Invalidate_Dcache_ClearTag(start);
        start += pcacheinfo->dcacheline_size;
    }

}

void os_arch_cache_init(void)
{
    unsigned int start, end;

    /* 1. identify cpu and probe cache */
    identify_cpu();
    probe_cache();

    start = K0BASE;
    end = (start + pcacheinfo->icache_size);

    /*
     *    2. clear CP0 taglo/taghi register;
     */
    pcacheop->Clear_TagLo();

    /*
     *    3. invalidate instruction cache;
     */
    while (start < end) {
        pcacheop->Invalidate_Icache(start); //index invalidate icache
        start += pcacheinfo->icacheline_size;
    }

    /*
     *    4. invalidate data cache;
     */
    start = K0BASE;
    end = (start + pcacheinfo->dcache_size);
    while (start < end) {
        pcacheop->Invalidate_Dcache_ClearTag(start);
        start += pcacheinfo->dcacheline_size;
    }

    start = K0BASE;
    while (start < end) {
        pcacheop->Invalidate_Dcache_Fill(start);  //index invalidate dcache
        start += pcacheinfo->dcacheline_size;
    }

    start = K0BASE;
    while (start < end) {
        pcacheop->Invalidate_Dcache_ClearTag(start);
        start += pcacheinfo->dcacheline_size;
    }

    /* enable cache */
    enable_os_arch_cache();
    printk("enable cpu cache done\n");

    return ;
}