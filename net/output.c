#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid) {
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
    int r;
    while (1) {
        r = ipc_recv(0, &nsipcbuf, 0);
        if (r < 0) {
            panic("output error: %e", r);
        }
        while ((r=sys_net_try_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0) {
            if (r != -E_TX_QUEUE_OVERFLOW) {
                panic("output error: %e", r);
            } else {
                sys_yield();
            }
        }
    }
}
