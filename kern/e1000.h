#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

#define NTXDESC        64
#define TXBUFSIZE      1518
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */

int e1000_attach(struct pci_func *pfp);
#endif	// JOS_KERN_E1000_H
