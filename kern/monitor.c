// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about the function backtrace", mon_backtrace },
	{ "showmappings", "Display the virtual address between input", mon_showmappings },
	{ "changePermission", "Change the permission in virtual address when using monitor", mon_changePermission },
	{ "dump", "Dump the contents of a range of memory given either a virtual or physical address range", mon_dump},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp;
    	int i;
	struct Eipdebuginfo info;

    	cprintf("Stack backtrace:\n");

    	ebp = read_ebp();

    	while (ebp != 0) {
        	cprintf("  ebp %08x", ebp);

        	cprintf("  eip %08x", *(uint32_t *)(ebp+4));

        	cprintf("  args");

       		for (i=0; i<5; i++) {
           	 cprintf(" %08x", *(uint32_t *)(ebp+8+i*4));
        	}
	 	if (debuginfo_eip(*(uint32_t *)(ebp+4), &info) < 0) {
            		cprintf("debuginfo_eip error");
 	           	return -1;
        	}

		cprintf("\n       %s:%d: %.*s+%d", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, *(uint32_t *)(ebp+4) - info.eip_fn_addr);

       		ebp = *(uint32_t *)ebp;
        	cprintf("\n");
    	}
	return 0;
}

// show the virtual memory mapping information
// between two virtual memory inputs
int
mon_showmappings(int argc, char **argv, struct Trapframe *tf) {
    uintptr_t vm1, vm2, p;
    pte_t *pte;

    if (argc != 3) {
        cprintf("Usage: %s HVP HVP(Hex virtual page)\n", argv[0]);
        return 0;
    }
    if (hptop(argv[1], &vm1) < 0) {
        cprintf("Usage: %s HVP HVP(Hex virtual page)\n", argv[0]);
        return 0;
    }
    if (hptop(argv[2], &vm2) < 0) {
        cprintf("Usage: %s HVP HVP(Hex virtual page)\n", argv[0]);
        return 0;
    }

    for (p=vm1; p<=vm2; p+=PGSIZE) {
        pte = pgdir_walk(kern_pgdir, (void *)p, 0);
        if (pte == NULL || (((*pte) & PTE_P) == 0)) {
            cprintf("Virtual page: %05x: no page mapping this virtual memory\n", p/16/16/16);
        } else {
            cprintf("Virtual page: %05x: %05x\n", p/16/16/16, PTE_ADDR(*pte)/16/16/16);
            cprintf("\tPTE_P: %d, PTE_W: %d, PTE_U: %d\n",
                            (*pte&PTE_P)==PTE_P, (*pte&PTE_W)==PTE_W, (*pte&PTE_U)==PTE_U);
        }
    }
    return 0;
}

int
mon_changePermission(int argc, char **argv, struct Trapframe *tf) {
    uintptr_t vm;
    pte_t *pte;
    int perm = 0;

    if (argc != 3) {
        cprintf("Usage %s: HVP(Hex Virtual Page) [UWP]*\n", argv[0]);
        return 0;
    }
    if (hptop(argv[1], &vm) < 0) {
        cprintf("Usage %s: HVP(Hex Virtual Page) [UWP]*\n", argv[0]);
        return 0;
    }

    if (strchr(argv[2], 'P') || strchr(argv[2], 'p')) {
        perm |= PTE_P;
    }
    if (strchr(argv[2], 'U') || strchr(argv[2], 'u')) {
        perm |= PTE_U;
    }
    if (strchr(argv[2], 'W') || strchr(argv[2], 'w')) {
        perm |= PTE_W;
    }

    pte = pgdir_walk(kern_pgdir, (void *)vm, 0);
    if (pte != NULL) {
        *pte = PTE_ADDR(*pte) | perm;
        cprintf("Changing permission ok!\n");
    }

    return 0;
}


int
mon_dump(int argc, char **argv, struct Trapframe *tf) {
    uintptr_t va;
    int unit, i;
    pte_t *pte;

    if (argc != 3) {
        cprintf("Usage %s: HVM(Hex Virtual Address) number-of-units\n", argv[0]);
        return 0;
    }
    if (htop(argv[1], &va) < 0) {
        cprintf("Usage %s: HVM(Hex Virtual Address) number-of-units\n", argv[0]);
        return 0;
    }
    if (atoi(argv[2], &unit) < 0) {
        cprintf("Usage %s: HVM(Hex Virtual Address) number-of-units\n", argv[0]);
        return 0;
    }

    for (i=0 ;i<unit; i++, va += 4) {
        if(i % 4 == 0) {
            cprintf("0x%08x:", va);
        }
        if((pte=pgdir_walk(kern_pgdir, (void *)va, 0)) || (*pte & PTE_P)==0) {
            cprintf(" 0x%08x", *(uint32_t *)va);
        } else {
            cprintf("  unmapped ");
        }
        if(i % 4 == 3) {
            cprintf("\n");
        }
    }

    if (i % 4 != 3) {
        cprintf("\n");
    }

    return 0;
}


/***** utility for monitor operation *****/
int
hptop(const char *hvp, uintptr_t *vm) {
    int i, len, k=16 * 16 * 16;
    uintptr_t sum = 0;

    if (strncmp("0x", hvp, 2) != 0 && strncmp("0X", hvp, 2) != 0) {
        return -1;
    }

    len = strlen(hvp);

    for (i=len-1; i>=2; i--) {
        if (hvp[i] >= '0' && hvp[i] <= '9') {
            sum += (hvp[i] - '0')*k;
        } else if (hvp[i]>='a' && hvp[i] <='f') {
            sum += (hvp[i] - 'a' + 10)*k;
        } else if (hvp[i]>='A' && hvp[i] <='F') {
            sum += (hvp[i] - 'A' + 10)*k;
        } else {
            return -1;
        }
        k *= 16;
    }

    *vm = sum;
    return 0;
}

int
htop(const char *hvp, uintptr_t *vm) {
    int i, len, k=1;
    uintptr_t sum = 0;

    if (strncmp("0x", hvp, 2) != 0 && strncmp("0X", hvp, 2) != 0) {
        return -1;
    }

    len = strlen(hvp);

    for (i=len-1; i>=2; i--) {
        if (hvp[i] >= '0' && hvp[i] <= '9') {
            sum += (hvp[i] - '0')*k;
        } else if (hvp[i]>='a' && hvp[i] <='f') {
            sum += (hvp[i] - 'a' + 10)*k;
        } else if (hvp[i]>='A' && hvp[i] <='F') {
            sum += (hvp[i] - 'A' + 10)*k;
        } else {
            return -1;
        }
        k *= 16;
    }

    *vm = sum;
    return 0;
}

int
atoi(const char *hvp, int *vm) {
    int i, len, k= 1;
    int sum = 0;

    len = strlen(hvp);

    for (i=len-1; i>=0; i--) {
        if (hvp[i] >= '0' && hvp[i] <= '9') {
            sum += (hvp[i] - '0')*k;
        } else {
            return -1;
        }
        k *= 10;
    }

    *vm = sum;
    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
