/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include "sc7.h"
#include "pmc.h"
#include "timer.h"

#define MAKE_CLK_RST_REG(r)                 (MAKE_REG32(((uintptr_t)0x60006000) + (r)))

#define CLK_RST_CONTROLLER_RST_DEV_H_SET_0  MAKE_CLK_RST_REG(0x308)
#define CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0  MAKE_CLK_RST_REG(0x32C)

#define MAKE_FLOW_CTLR_REG(r)               (MAKE_REG32(((uintptr_t)0x60007000) + (r)))

#define FLOW_CTLR_HALT_COP_EVENTS_0         MAKE_FLOW_CTLR_REG(0x004)

#define MAKE_AHBDMA_REG(r)                  (MAKE_REG32(((uintptr_t)0x60008000) + (r)))

#define AHBDMA_CMD_0                        MAKE_AHBDMA_REG(0x000)

#define MAKE_MC_REG(r)                      (MAKE_REG32(((uintptr_t)0x70019000) + (r)))

#define MC_SMMU_TLB_CONFIG_0                MAKE_MC_REG(0x014)
#define MC_SMMU_PTB_ASID_0                  MAKE_MC_REG(0x01C)
#define MC_SMMU_PTB_DATA_0                  MAKE_MC_REG(0x020)
#define MC_SMMU_TLB_FLUSH_0                 MAKE_MC_REG(0x030)
#define MC_SMMU_PTC_FLUSH_0                 MAKE_MC_REG(0x034)

#define AVP_CACHE_CONFIG_0                  MAKE_REG32((uintptr_t)0x50040000)

#define PGE_DIR                             ((uintptr_t)0x80000000)
#define PGE_TBL                             ((uintptr_t)0x80001000)
#define PGE                                 ((uintptr_t)0x80002000)

#define MEMCPY32(d,s,n)                                                  \
  for (unsigned int i = 0; i < (n); i += 4) {                            \
    MAKE_REG32(((uintptr_t)(d)) + i) = MAKE_REG32(((uintptr_t)(s)) + i); \
  }

#define MEMSET32(s,c,n)                               \
  for (unsigned int i = 0; i < (n); i += 4) {         \
    MAKE_REG32(((uintptr_t)(s)) + i) = (uint32_t)(c); \
  }

void reboot(void) {
  APBDEV_PMC_CNTRL_0 = 0x10;

  for (;;) {
  }
}

void prelude(void) {
  AVP_CACHE_CONFIG_0 = 0xC00;

  MEMSET32(PGE_DIR, 0, 0x3000);

  MAKE_REG32(PGE_DIR + ((PGE >> 20) & 0xFFC)) = 0xF0000000 | (PGE_TBL >> 12);
  MAKE_REG32(PGE_TBL + ((PGE >> 10) & 0xFFC)) = 0xE0000000 | (PGE     >> 12);

  MC_SMMU_PTB_ASID_0 = 0;
  (void)MC_SMMU_TLB_CONFIG_0;

  MC_SMMU_PTB_DATA_0 = 0xE0000000 | (PGE_DIR >> 12);
  (void)MC_SMMU_TLB_CONFIG_0;

  MC_SMMU_PTC_FLUSH_0 = 0;
  (void)MC_SMMU_TLB_CONFIG_0;

  MC_SMMU_TLB_FLUSH_0 = 0;
  (void)MC_SMMU_TLB_CONFIG_0;

  for (;;) {
    FLOW_CTLR_HALT_COP_EVENTS_0 = 0x50000000;
  }
}

void sc7_entry_main(void) {
  APBDEV_PMC_DPD_ENABLE_0 = 0;

  spinlock_wait(0x128);

  AHBDMA_CMD_0 &= ~BIT(31);

  CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = BIT(1);
  CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 = BIT(1);

  volatile struct _params {
    uint32_t phys;
    uint32_t size;
  } *params = (volatile struct _params *)0x4003DFF8;

  MEMCPY32(0x40010000, params->phys, params->size);

  APBDEV_PMC_SCRATCH45_0 = 0x2E38DFFF;
  APBDEV_PMC_SCRATCH46_0 = 0x6001DC28;

  APBDEV_PMC_SCRATCH33_0 = 0x4003F000;
  APBDEV_PMC_SCRATCH40_0 = 0x6000F208;

  reboot();
}

