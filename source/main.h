#include "sc7fw_bin.h"
#include "rebootstub_bin.h"

#define UNUSED                              __attribute__((unused))

#define MAKE_REG32(a)                       (*(volatile uint32_t *)(a))

#define IRAM_BASE                           0x40000000

#define IRAM(a)                             (MAKE_REG32(g_iram_base + (a)))

#define CLK_RST_BASE                        0x60006000

#define MAKE_CLK_RST_REG(r)                 (MAKE_REG32(g_clk_rst_base + (r)))

#define CLK_RST_CONTROLLER_RST_DEV_L_SET_0  MAKE_CLK_RST_REG(0x300)
#define CLK_RST_CONTROLLER_RST_DEV_L_CLR_0  MAKE_CLK_RST_REG(0x304)
#define CLK_RST_CONTROLLER_RST_DEV_H_SET_0  MAKE_CLK_RST_REG(0x308)
#define CLK_RST_CONTROLLER_RST_DEV_H_CLR_0  MAKE_CLK_RST_REG(0x30C)
#define CLK_RST_CONTROLLER_CLK_ENB_H_SET_0  MAKE_CLK_RST_REG(0x328)
#define CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0  MAKE_CLK_RST_REG(0x32C)

#define FLOW_CTLR_BASE                      0x60007000

#define MAKE_FLOW_CTLR_REG(r)               (MAKE_REG32(g_flow_ctlr_base + (r)))

#define FLOW_CTLR_HALT_COP_EVENTS_0         MAKE_FLOW_CTLR_REG(0x004)

#define AHBDMA_BASE                         0x60008000

#define MAKE_AHBDMA_REG(r)                  (MAKE_REG32(g_ahbdma_base + (r)))

#define AHBDMA_CMD_0                        MAKE_AHBDMA_REG(0x000)
#define AHBDMA_STA_0                        MAKE_AHBDMA_REG(0x004)
#define AHBDMA_TX_REQ_0                     MAKE_AHBDMA_REG(0x008)
#define AHBDMA_COUNTER_0                    MAKE_AHBDMA_REG(0x010)

#define AHBDMACHAN_BASE                     0x60009000

#define MAKE_AHBDMACHAN_n_REG(n,r)          (MAKE_REG32(g_ahbdmachan_base + \
                                            (0x20 * (n)) + (r)))

#define AHBDMACHAN_CHANNEL_0_CSR_0          MAKE_AHBDMACHAN_n_REG(0 ,0x000)
#define AHBDMACHAN_CHANNEL_0_STA_0          MAKE_AHBDMACHAN_n_REG(0, 0x004)
#define AHBDMACHAN_CHANNEL_0_AHB_PTR_0      MAKE_AHBDMACHAN_n_REG(0, 0x010)
#define AHBDMACHAN_CHANNEL_0_AHB_SEQ_0      MAKE_AHBDMACHAN_n_REG(0, 0x014)
#define AHBDMACHAN_CHANNEL_0_XMB_PTR_0      MAKE_AHBDMACHAN_n_REG(0, 0x018)
#define AHBDMACHAN_CHANNEL_0_XMB_SEQ_0      MAKE_AHBDMACHAN_n_REG(0, 0x01C)

#define AHBDMACHAN_CHANNEL_1_CSR_0          MAKE_AHBDMACHAN_n_REG(1, 0x000)
#define AHBDMACHAN_CHANNEL_1_STA_0          MAKE_AHBDMACHAN_n_REG(1, 0x004)
#define AHBDMACHAN_CHANNEL_1_AHB_PTR_0      MAKE_AHBDMACHAN_n_REG(1, 0x010)
#define AHBDMACHAN_CHANNEL_1_AHB_SEQ_0      MAKE_AHBDMACHAN_n_REG(1, 0x014)
#define AHBDMACHAN_CHANNEL_1_XMB_PTR_0      MAKE_AHBDMACHAN_n_REG(1, 0x018)
#define AHBDMACHAN_CHANNEL_1_XMB_SEQ_0      MAKE_AHBDMACHAN_n_REG(1, 0x01C)

#define AHBDMACHAN_CHANNEL_2_CSR_0          MAKE_AHBDMACHAN_n_REG(2, 0x000)
#define AHBDMACHAN_CHANNEL_2_STA_0          MAKE_AHBDMACHAN_n_REG(2, 0x004)
#define AHBDMACHAN_CHANNEL_2_AHB_PTR_0      MAKE_AHBDMACHAN_n_REG(2, 0x010)
#define AHBDMACHAN_CHANNEL_2_AHB_SEQ_0      MAKE_AHBDMACHAN_n_REG(2, 0x014)
#define AHBDMACHAN_CHANNEL_2_XMB_PTR_0      MAKE_AHBDMACHAN_n_REG(2, 0x018)
#define AHBDMACHAN_CHANNEL_2_XMB_SEQ_0      MAKE_AHBDMACHAN_n_REG(2, 0x01C)

#define AHBDMACHAN_CHANNEL_3_CSR_0          MAKE_AHBDMACHAN_n_REG(3, 0x000)
#define AHBDMACHAN_CHANNEL_3_STA_0          MAKE_AHBDMACHAN_n_REG(3, 0x004)
#define AHBDMACHAN_CHANNEL_3_AHB_PTR_0      MAKE_AHBDMACHAN_n_REG(3, 0x010)
#define AHBDMACHAN_CHANNEL_3_AHB_SEQ_0      MAKE_AHBDMACHAN_n_REG(3, 0x014)
#define AHBDMACHAN_CHANNEL_3_XMB_PTR_0      MAKE_AHBDMACHAN_n_REG(3, 0x018)
#define AHBDMACHAN_CHANNEL_3_XMB_SEQ_0      MAKE_AHBDMACHAN_n_REG(3, 0x01C)

#define EXCEPTION_VECTOR_BASE               0x6000F000

#define BPMP_VECTOR_RESET                   (EXCEPTION_VECTOR_BASE + 0x200)
#define BPMP_VECTOR_UNDEF                   (EXCEPTION_VECTOR_BASE + 0x204)
#define BPMP_VECTOR_SWI                     (EXCEPTION_VECTOR_BASE + 0x208)
#define BPMP_VECTOR_PREFETCH_ABORT          (EXCEPTION_VECTOR_BASE + 0x20C)
#define BPMP_VECTOR_DATA_ABORT              (EXCEPTION_VECTOR_BASE + 0x210)
#define BPMP_VECTOR_RESERVED                (EXCEPTION_VECTOR_BASE + 0x214)
#define BPMP_VECTOR_IRQ                     (EXCEPTION_VECTOR_BASE + 0x218)
#define BPMP_VECTOR_FIQ                     (EXCEPTION_VECTOR_BASE + 0x21C)

#define MC_BASE                             0x70019000

#define MC_SMMU_PTB_ASID_0                  (MC_BASE + 0x01C)
#define MC_SMMU_PTB_DATA_0                  (MC_BASE + 0x020)
#define MC_SMMU_TLB_FLUSH_0                 (MC_BASE + 0x030)
#define MC_SMMU_PTC_FLUSH_0                 (MC_BASE + 0x034)
#define MC_SMMU_AVPC_ASID_0                 (MC_BASE + 0x23C)
#define MC_SMMU_PPCS_ASID_0                 (MC_BASE + 0x270)

#define COPY_TO_IRAM(d,s,n)                           \
  for (unsigned int i = 0; i < (n); i += 4) {         \
    uint32_t tmp = *(uint32_t *)((uintptr_t)(s) + i); \
    IRAM((d) + i) = tmp;                              \
  }

