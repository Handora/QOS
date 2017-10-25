#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here
struct tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

volatile uint32_t *eBar;
struct tx_desc *txDescRing[NTXDESC];
uint32_t *txBuf[TXBUFSIZE];

void
transmit_init() {
    memset(txDescRing, 0, sizeof(txDescRing));
    memset(txBuf, 0, sizeof(txBuf));
    eBar[E1000_TDBAL] =
    eBar[E1000_TDLEN] = sizeof(txDescRing);
}

int
e1000_attach(struct pci_func *pfp) {
    pci_func_enable(pfp);
    eBar = mmio_map_region(pfp->reg_base[0], pfp->reg_size[0]);
    cprintf("Device Status Register: 0x%x\n", (uint32_t)*(eBar+8/4));
    return 1;
}

