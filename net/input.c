#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
    int r;

    while (1) {
        nsipcbuf.pkt.jp_len = PGSIZE - 4;
        while ((r=sys_net_try_receive(nsipcbuf.pkt.jp_data, &nsipcbuf.pkt.jp_len)) < 0) {
            if (r != -E_RX_QUEUE_EMPTY) {
                panic("input error: %e", r);
            }
            sys_yield();
        }
        ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_U | PTE_W | PTE_P);
        sys_yield();
        sys_yield();
    }
}
