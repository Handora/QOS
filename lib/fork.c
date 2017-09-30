// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    if ((err & FEC_WR) == 0) {
        panic("The fault wasn't a write");
    }

    if ((uvpt[PGNUM(addr)] & PTE_COW) == 0) {
        panic("The page isn't COPY-ON-WRITE");
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
    addr = ROUNDDOWN(addr, PGSIZE);
    if ((r = sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P)) < 0) {
        panic("sys_page_alloc error: %e", r);
    }
    memcpy(PFTEMP, addr, PGSIZE);
    if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P)) < 0) {
        panic("sys_page_map error: %e", r);
    }
    if ((r = sys_page_unmap(0, PFTEMP)) < 0) {
        panic("sys_page_unmap error: %e", r);
    }
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
    int perm;
    uintptr_t addr = pn*PGSIZE;

	// LAB 4: Your code here.
    if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
        r = sys_page_map(0, (void *)addr, envid, (void *)addr, PTE_COW|PTE_U|PTE_P);
        if (r < 0) {
            return r;
        }

        r = sys_page_map(envid, (void *)addr, 0, (void *)addr, PTE_COW|PTE_U|PTE_P);
        if (r < 0) {
            return r;
        }

    } else {
        r = sys_page_map(0, (void *)addr, envid, (void *)addr, PTE_U|PTE_P);
        if (r < 0) {
            return r;
        }
    }
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
    int r;
    int pid;
    uintptr_t addr;
	// LAB 4: Your code here.
    set_pgfault_handler(pgfault);
    if ((pid = sys_exofork()) < 0) {
        return pid;
    }
    if (pid > 0) {
        for (addr = 0; addr < USTACKTOP; addr+=PGSIZE) {
            if (!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[PGNUM(addr)] & PTE_P)) {
                continue;
            }
            if ((r=duppage(pid, PGNUM(addr))) < 0) {
                return r;
            }
        }
        if ((r = sys_page_alloc(pid, (void *)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P)) < 0) {
            return r;
        }
        extern void _pgfault_upcall();
        if ((r = sys_env_set_pgfault_upcall(pid, _pgfault_upcall)) < 0) {
            return r;
        }
        if ((r = sys_env_set_status(pid, ENV_RUNNABLE)) < 0) {
            return r;
        }
    } else {
        thisenv = &envs[ENVX(sys_getenvid())];
    }
    return pid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
