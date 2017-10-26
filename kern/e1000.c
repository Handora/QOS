#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

// LAB 6: Your driver code here
volatile uint32_t *eBar;
struct tx_desc txDescRing[NTXDESC];
struct tx_pkt txBuf[NTXDESC];

void
transmit_init() {
    int i;

    memset(txDescRing, 0, sizeof(txDescRing));
    memset(txBuf, 0, sizeof(txBuf));

    for (i=0; i<NTXDESC; i++) {
        txDescRing[i].addr = PADDR((void *)(txBuf+i));
        txDescRing[i].cmd |= 0x9;
        txDescRing[i].status |= 0x1;
    }

    eBar[E1000_TDBAL/4] = PADDR(txDescRing);
    eBar[E1000_TDBAH/4] = 0;
    eBar[E1000_TDLEN/4] = sizeof(struct tx_desc)*NTXDESC;
    eBar[E1000_TDH/4] = eBar[E1000_TDT/4] = 0;
    eBar[E1000_TCTL/4] = 0 | E1000_TCTL_EN | E1000_TCTL_PSP;
    eBar[E1000_TCTL/4] |= E1000_TCTL_CT & (0x10 << 4);
    eBar[E1000_TCTL/4] |= E1000_TCTL_COLD & (0x40 << 12);
    eBar[E1000_TIPG/4] = 0 | (E1000_TIPG_IPGT & (10));
    eBar[E1000_TIPG/4] |= E1000_TIPG_IPGR1 & (4 << 10);
    eBar[E1000_TIPG/4] |= E1000_TIPG_IPGR2 & (6 << 20);
}

int
check_for_transmit(struct tx_desc *desc) {
    return (desc->cmd & 0x8) && (desc->status & 0x1);
}

int
transmit(const char *buf, int len) {
    struct tx_desc *desc = &txDescRing[eBar[E1000_TDT/4]];
    struct tx_pkt *txbuf = &txBuf[eBar[E1000_TDT/4]];
    if (!check_for_transmit(desc)) {
        return -E_TX_QUEUE_OVERFLOW;
    }
    len = len > TXBUFSIZE ? TXBUFSIZE : len;
    memmove(txbuf->buf, buf, len);
    desc->length = len;
    desc->cmd |= 0x8;
    desc->status &= ~0x1;
    if (eBar[E1000_TDT/4] == NTXDESC-1) {
        eBar[E1000_TDT/4] = 0;
    } else {
        eBar[E1000_TDT/4] += 1;
    }
    return 0;
}

int
e1000_attach(struct pci_func *pfp) {
    pci_func_enable(pfp);
    eBar = mmio_map_region(pfp->reg_base[0], pfp->reg_size[0]);
    cprintf("Device Status Register: 0x%x\n", (uint32_t)*(eBar+8/4));
    transmit_init();
    return 1;
}

