#include <stdio.h>
#include <stdint.h>
#include <stdalign.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <switch.h>

#include "defines.h"
#include "sc7fw_bin.h"
#include "rebootstub_bin.h"

#define MODULE_CAFF      207
#define DeviceName_PPCS  10

static const char *g_payload_path = "sdmc:/atmosphere/reboot_payload.bin";

static uint32_t alignas(0x1000) g_device_pages[0x9400];

static uintptr_t g_iram_base;
static uintptr_t g_clk_rst_base;
static uintptr_t g_flow_ctlr_base;
static uintptr_t g_ahbdma_base;
static uintptr_t g_ahbdmachan_base;

uint32_t __nx_applet_type = AppletType_None;

void __libnx_initheap(void) {
  static char fake_heap[0x40000];

  extern const void *fake_heap_start;
  extern const void *fake_heap_end;

  fake_heap_start = &fake_heap[0];
  fake_heap_end   = &fake_heap[sizeof fake_heap];
}

void __appInit(void) {
  Result rc;

  rc = smInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
  }

  rc = setsysInitialize();
  if (R_SUCCEEDED(rc)) {
    SetSysFirmwareVersion fw;

    rc = setsysGetFirmwareVersion(&fw);
    if (R_SUCCEEDED(rc)) {
      hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
    }
    setsysExit();
  }

  rc = fsInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
  }

  rc = fsdevMountSdmc();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
  }

  rc = pmshellInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = pscInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }
}

void __appExit(void) {
  pscExit();
  pmshellExit();
  fsdevUnmountAll();
  fsExit();
  smExit();
}

static inline void query_io_mappings(void) {
  Result rc;

  rc = svcQueryIoMapping(&g_iram_base, IRAM_BASE, 0x40000);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = svcQueryIoMapping(&g_clk_rst_base, CLK_RST_BASE, 0x1000);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = svcQueryIoMapping(&g_flow_ctlr_base, FLOW_CTLR_BASE, 0x1000);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = svcQueryIoMapping(&g_ahbdma_base, AHBDMA_BASE, 0x1000);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = svcQueryIoMapping(&g_ahbdmachan_base, AHBDMACHAN_BASE, 0x1000);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }
}

static inline void ahbdma_global_enable(void) {
  AHBDMA_CMD_0 |= BIT(31);
}

static inline void ahbdma_global_disable(void) {
  AHBDMA_CMD_0 &= ~BIT(31);
}

static void ahbdma_init_hw(void) {
  CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = BIT(1);
  CLK_RST_CONTROLLER_CLK_ENB_H_SET_0 = BIT(1);

  svcSleepThread(1000000ull);

  CLK_RST_CONTROLLER_RST_DEV_H_CLR_0 = BIT(1);
#if 0
  ahbdma_global_enable();
#else
  ahbdma_global_disable();
#endif
}

static UNUSED void ahbdma_deinit_hw(void) {
  ahbdma_global_disable();

  CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = BIT(1);
  CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 = BIT(1);
}

static void ahbdma_write_reg32(uint32_t phys, uint32_t data) {
  IRAM(0x3FFDC) = data;

  AHBDMACHAN_CHANNEL_0_AHB_SEQ_0 = 0x02000000;

  AHBDMACHAN_CHANNEL_0_AHB_PTR_0 = 0x4003FFDC;
  AHBDMACHAN_CHANNEL_0_XMB_PTR_0 = 0x80000100;

  AHBDMACHAN_CHANNEL_0_CSR_0 = BIT(31) | BIT(27) | BIT(26);

  while (AHBDMACHAN_CHANNEL_0_STA_0 & BIT(31)) {
  }

  AHBDMACHAN_CHANNEL_0_AHB_PTR_0 = phys;
  AHBDMACHAN_CHANNEL_0_XMB_PTR_0 = 0x80000100;

  AHBDMACHAN_CHANNEL_0_CSR_0 = BIT(31) | BIT(26);

  while (AHBDMACHAN_CHANNEL_0_STA_0 & BIT(31)) {
  }
}

static UNUSED uint32_t ahbdma_read_reg32(uint32_t phys) {
  IRAM(0x3FFDC) = 0xCAFEBABE;

  AHBDMACHAN_CHANNEL_0_AHB_SEQ_0 = 0x02000000;

  AHBDMACHAN_CHANNEL_0_AHB_PTR_0 = phys;
  AHBDMACHAN_CHANNEL_0_XMB_PTR_0 = 0x80000100;

  AHBDMACHAN_CHANNEL_0_CSR_0 = BIT(31) | BIT(27) | BIT(26);

  while (AHBDMACHAN_CHANNEL_0_STA_0 & BIT(31)) {
  }

  AHBDMACHAN_CHANNEL_0_AHB_PTR_0 = 0x4003FFDC;
  AHBDMACHAN_CHANNEL_0_XMB_PTR_0 = 0x80000100;

  AHBDMACHAN_CHANNEL_0_CSR_0 = BIT(31) | BIT(26);

  while (AHBDMACHAN_CHANNEL_0_STA_0 & BIT(31)) {
  }

  return IRAM(0x3FFDC);
}

static void ahbdma_prepare_for_sleep(void) {
  g_device_pages[0x9000] = 0x40038000;
  g_device_pages[0x9004] = 0x40038000;
  g_device_pages[0x9008] = 0x40038000;
  g_device_pages[0x900C] = 0x40038000;
  g_device_pages[0x9010] = 0x40038000;
  g_device_pages[0x9014] = 0x40038000;
  g_device_pages[0x9018] = 0x40038000;
  g_device_pages[0x901C] = 0x40038000;

  AHBDMACHAN_CHANNEL_2_AHB_PTR_0 = BPMP_VECTOR_RESET;
  AHBDMACHAN_CHANNEL_2_AHB_SEQ_0 = 0x02000000;
  AHBDMACHAN_CHANNEL_2_XMB_PTR_0 = 0x80024000;

  AHBDMACHAN_CHANNEL_2_CSR_0 = BIT(31) | (7ul << 2);
}

static UNUSED void execute_on_bpmp(uint32_t entry) {
  ahbdma_write_reg32(BPMP_VECTOR_RESET, entry);

  CLK_RST_CONTROLLER_RST_DEV_L_SET_0 = BIT(1);
  svcSleepThread(2000ull);
  CLK_RST_CONTROLLER_RST_DEV_L_CLR_0 = BIT(1);

  FLOW_CTLR_HALT_COP_EVENTS_0 = 0;

  while ((FLOW_CTLR_HALT_COP_EVENTS_0 >> 29) != 2) {
    svcSleepThread(1000ull);
  }
}

int main(void) {
  Result rc;

  query_io_mappings();
  ahbdma_init_hw();

  FILE *f;
  if (!(f = fopen(g_payload_path, "rb"))) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  struct stat st;
  if ((stat(g_payload_path, &st) != 0) || !S_ISREG(st.st_mode)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  if (!st.st_size || ((uint64_t)st.st_size > 0x24000)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  if (fread(g_device_pages, 1, st.st_size, f) != (size_t)st.st_size) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  fclose(f);

  armDCacheFlush(g_device_pages, sizeof g_device_pages);

  COPY_TO_IRAM(0x38000, sc7fw_bin, sc7fw_bin_size);
  COPY_TO_IRAM(0x38800, rebootstub_bin, rebootstub_bin_size);

  Handle dev_as_h;
  rc = svcCreateDeviceAddressSpace(&dev_as_h, 0, 0x100000000ull);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = svcMapDeviceAddressSpaceByForce(
    dev_as_h, CUR_PROCESS_HANDLE, (uintptr_t)g_device_pages,
    sizeof g_device_pages, 0x80000000ull, Perm_R
  );
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = pmshellTerminateProcessByTitleId(0x0100000000000015ull);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  PscPmModule pm_module;
  const uint16_t pm_dependencies[] = { 0x18, 0x1F };

  rc = pscGetPmModule(&pm_module, 0x29, pm_dependencies, 2, true);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  ahbdma_prepare_for_sleep();

  Waiter pm_request_w = waiterForEvent(&pm_module.event);
  while (true) {
    rc = waitSingle(pm_request_w, UINT64_MAX);
    if (R_FAILED(rc)) {
      continue;
    }

    PscPmState pm_state;
    uint32_t flags;

    rc = pscPmModuleGetRequest(&pm_module, &pm_state, &flags);
    if (R_FAILED(rc)) {
      continue;
    }

    switch (pm_state) {
      case PscPmState_Awake:
      case PscPmState_ReadyAwaken:
      case PscPmState_ReadySleep:
      case PscPmState_ReadyAwakenCritical:
      case PscPmState_ReadyShutdown:
        break;
      case PscPmState_ReadySleepCritical: {
        do {
          rc = svcAttachDeviceAddressSpace(DeviceName_PPCS, dev_as_h);
        } while (R_FAILED(rc));
        ahbdma_global_enable();
        break;
      }
    }
    pscPmModuleAcknowledge(&pm_module, pm_state);
  }
}

