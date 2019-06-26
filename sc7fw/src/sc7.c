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

#include <string.h>
#include <stdnoreturn.h>

#include "sc7.h"
#include "pmc.h"
#include "car.h"
#include "i2c.h"

#define AVP_CACHE_CONFIG_0              MAKE_REG32((uintptr_t)0x50040000)

#define MAKE_AHBDMA_REG(r)              (MAKE_REG32(((uintptr_t)0x60008000) + (r)))

#define AHBDMA_CMD_0                    MAKE_AHBDMA_REG(0x000)
#define AHBDMA_STA_0                    MAKE_AHBDMA_REG(0x004)

#define MAKE_AHBDMACHAN_n_REG(n,r)      (MAKE_REG32(((uintptr_t)0x60009000) + \
                                        (0x20 * (n)) + (r)))

#define AHBDMACHAN_CHANNEL_0_CSR_0      MAKE_AHBDMACHAN_n_REG(0 ,0x000)
#define AHBDMACHAN_CHANNEL_0_AHB_PTR_0  MAKE_AHBDMACHAN_n_REG(0, 0x010)
#define AHBDMACHAN_CHANNEL_0_AHB_SEQ_0  MAKE_AHBDMACHAN_n_REG(0, 0x014)
#define AHBDMACHAN_CHANNEL_0_XMB_PTR_0  MAKE_AHBDMACHAN_n_REG(0, 0x018)

#define AHBDMACHAN_CHANNEL_1_CSR_0      MAKE_AHBDMACHAN_n_REG(1, 0x000)
#define AHBDMACHAN_CHANNEL_1_AHB_PTR_0  MAKE_AHBDMACHAN_n_REG(1, 0x010)
#define AHBDMACHAN_CHANNEL_1_AHB_SEQ_0  MAKE_AHBDMACHAN_n_REG(1, 0x014)
#define AHBDMACHAN_CHANNEL_1_XMB_PTR_0  MAKE_AHBDMACHAN_n_REG(1, 0x018)

#define AHBDMACHAN_CHANNEL_2_CSR_0      MAKE_AHBDMACHAN_n_REG(2 ,0x000)
#define AHBDMACHAN_CHANNEL_3_CSR_0      MAKE_AHBDMACHAN_n_REG(3, 0x000)

noreturn void reboot(void) {
  APBDEV_PMC_CNTRL_0 = 0x10;

  for (;;) {
  }
}

static inline void ahbdma_copy_payload_to_iram(void) {
  AHBDMACHAN_CHANNEL_0_CSR_0 = 0;
  AHBDMACHAN_CHANNEL_1_CSR_0 = 0;
  AHBDMACHAN_CHANNEL_2_CSR_0 = 0;
  AHBDMACHAN_CHANNEL_3_CSR_0 = 0;

  while (AHBDMA_STA_0 & (0xFu << 24)) {
  }

  AHBDMACHAN_CHANNEL_0_AHB_PTR_0 = 0x40010000;
  AHBDMACHAN_CHANNEL_0_AHB_SEQ_0 = 0x02000000;
  AHBDMACHAN_CHANNEL_0_XMB_PTR_0 = 0x80000000;

  AHBDMACHAN_CHANNEL_0_CSR_0 = BIT(31) | BIT(26) | (0x3FFFu << 2);

  AHBDMACHAN_CHANNEL_1_AHB_PTR_0 = 0x40020000;
  AHBDMACHAN_CHANNEL_1_AHB_SEQ_0 = 0x02000000;
  AHBDMACHAN_CHANNEL_1_XMB_PTR_0 = 0x80010000;

  AHBDMACHAN_CHANNEL_1_CSR_0 = BIT(31) | BIT(26) | (0x3FFFu << 2);

  while (AHBDMA_STA_0 & (0x3u << 24)) {
  }

  AHBDMA_CMD_0 &= ~BIT(31);
}

static inline void configure_hiz_mode(void) {
  clkrst_reboot(CARDEVICE_I2C1);

  i2c_init(I2C_1);
  i2c_clear_ti_charger_bit_7();

  clkrst_disable(CARDEVICE_I2C1);
}

static inline void configure_pmic_wake_event(void) {
  clkrst_reboot(CARDEVICE_I2C5);

  i2c_init(I2C_5);
  i2c_set_pmic_wk_en0();

  clkrst_disable(CARDEVICE_I2C5);
}

void sc7_entry_main(void) {
  AVP_CACHE_CONFIG_0 |= 0xC00;

  while (APBDEV_PMC_PWRGATE_STATUS_0 & 1) {
  }

  ahbdma_copy_payload_to_iram();

  configure_pmic_wake_event();
  configure_hiz_mode();

  APBDEV_PMC_SCRATCH45_0 = 0x2E38DFFF;
  APBDEV_PMC_SCRATCH46_0 = 0x6001DC28;

  APBDEV_PMC_SCRATCH33_0 = 0x40038800;
  APBDEV_PMC_SCRATCH40_0 = 0x6000F208;

  reboot();
}

