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
#include "timer.h"
#include "utils.h"

#define AVP_CACHE_CONFIG_0  MAKE_REG32((uintptr_t)0x50040000)

noreturn void reboot(void) {
  APBDEV_PMC_CNTRL_0 = 0x10;

  for (;;) {
  }
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

  /* FIXME: determine that our payload is in place rather than stupidly waiting */
  /* if your display remains blank, try adjusting this value */
  spinlock_wait(0x1000000);

  configure_pmic_wake_event();
  configure_hiz_mode();

  APBDEV_PMC_SCRATCH45_0 = 0x2E38DFFF;
  APBDEV_PMC_SCRATCH46_0 = 0x6001DC28;

  APBDEV_PMC_SCRATCH33_0 = 0x40038800;
  APBDEV_PMC_SCRATCH40_0 = 0x6000F208;

  reboot();
}

