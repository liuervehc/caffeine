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

.section .text.start
.align 4

.globl _start

_start:
  b   _crt0

.globl _reboot
  b   reboot

.globl _prelude
  b   prelude

.globl _crt0
.type  _crt0,%function

_crt0:
  msr   cpsr_cxsf,#0xD3
  ldr   sp,=__stack_top__
  bl    sc7_entry_main
  b     reboot

.globl spinlock_wait
.type  spinlock_wait,%function

spinlock_wait:
  subs  r0,r0,#1
  bgt   spinlock_wait
  bx    lr

