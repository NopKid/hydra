#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>


/* CVE-2023-21492 Simulation
*
* Samsubg Kernel wrote kernel pointers into log files.
* Any app with READ_LOGS permission could read these.
* The log file path was accessible to unprivileged processes.
*
* This simulates both sides:
* 1. The vulnerable kernel writing pointers to a log.
* 2. The attacker reading the log and extracting addresses
*/

#define LOG_FILE "/tmp/simulated_kernel.log"

/* ============================================
 * SIDE 1: Simulate vulnerable kernel logging
 * ============================================ */

typedef struct {
  int pid;
  void *stack_ptr;
  void *heap_ptr;
  void *code_ptr;

} kernel_object;

void vulnerable_kernel_log(kernel_object *obj) {
  FILE *log = fopen(LOG_FILE, "a");
  if (!log) return;


  /* Get current timestamp */
  time_t now = time(NULL);
  char *ts = ctime(&now);
  ts[strlen(ts)-1] = '\0';


  /* This is the Vulnerability
  * Kernel writes raw pointer values into the log
  * format: [timestamp] [pid] [function] ptr=0xADDRESS
  *
  * The vulnerable logs lines looks similar to that of samsung:
  * "slub_debug: object 0xffffffc012345678 allocated"
  * ion_buffer: handle 0xffffffc098765432 create   */

    fprintf(log, "[%s] pid=%d\n stack_ptr=%p heap_ptr=%p\n",
           ts, obj->pid, obj->stack_ptr, obj->heap_ptr);
    fprintf(log, "[%s] Kernel_object allocated at %p size=%zu\n",
            ts, obj, sizeof(kernel_object));


    fclose(log);


}

void simulate_kernel_activity()
{
  printf("[*] Simulating kernel activity - writing logs...\n\n");

  /* Simulating multiple kernel objects being logged */
  for (int i = 0; i < 5; i++) {
    kernel_object *obj = malloc(sizeof(kernel_object));
    obj->pid        = 1000 + i;
    obj->stack_ptr  = (void*)(0xffffffc000000000 + rand() % 0x10000000);
    obj->heap_ptr   = (void*)(0xffffffc010000000 + rand() % 0x10000000);
    obj->code_ptr   = (void*)(0xffffffc020000000 + rand() % 0x10000000);

    vulnerable_kernel_log(obj);
    free(obj);

  }

  printf("[+] Log written to %s\n\n", LOG_FILE);

}

/* ============================================
 * SIDE 2: Simulate attacker reading the log
 * ============================================ */

/* Check if a string looks like a kernel address
 * Android kernel addresses on ARM64 start with 0xffffffc */

int is_kernel_addr(const char *token)
{
  if (strncmp(token, "0xffffffc", 8) == 0 && strlen(token) >= 18) {
    return 1;

  }
  /* Also check for pointers = format */
  char *ptr_loc = strstr(token, "=0");
  if(ptr_loc && strlen(ptr_loc) > 12) {
    return 1;
  }
  
  return 0;


}

/* Parse a hex address string into a number */
uintptr_t parse_addr(const char *token) {
  /* Find the 0x part */
  char *start = strstr(token, "0x");
  if (!start) {
    start = strstr(token, "=0");
    if (start) start += 1; /* Skip the = */

  }
  if (!start) return 0;

  return (uintptr_t)strtoul(start, NULL, 16);

}

void attacker_reads_log()
{
  printf("[+] Attacker reading log file...\n\n");


  FILE *log = fopen(LOG_FILE, "r");
  if (!log) {
    printf("[-] Log not found\n");
    return;

  }
  char line[512];
  uintptr_t leaked_addrs[64];
  int count = 0;

  /* Read log line by line */
  while (fgets(line, sizeof(line), log) && count < 64) {
    /* Tokenize each line by spaces */
    char *token = strtok(line, " \t\n");


    while (token) {
      if(is_kernel_addr(token)) {
        uintptr_t addr = parse_addr(token);
        if (addr != 0) {
          leaked_addrs[count++] = addr;
          printf("  [!] Leaked address found: 0x%x\n", addr);
        }
      }
      token = strtok(NULL, " \t\n");

    }
  }
  fclose (log);


  if (count == 0) {
    printf("[-] No kernel addresses found\n");
    return;
  }

  /* Find the lowest address, likely closest to kernel base */
  uintptr_t lowest = leaked_addrs[0];
  for (int i = 1; i < count; i++) {
    if (leaked_addrs[i] < lowest) {
      lowest = leaked_addrs[i];

    }
  }
  printf("\n[+] Total leaked addresses : %d\n", count);
  printf("[+] Lowest (nearest base)    : 0x%lx\n", lowest);

  /* Looking up the symbol table of the samsung kernel ( from samsung's open source portal) */
  printf("\n[*] If known_offset = 0x1234560:\n");
  printf("  Kernel base = 0x%lx\n", lowest - 0x1234560);
  printf("  KASLR       = DEFEATED\n");

}

int main() {
    srand(42);  /* fixed seed for reproducibility */

    printf("\n");
    printf("============================================\n");
    printf("  CVE-2023-21492 Simulation                \n");
    printf("  Kernel Pointer Leak via Log Files        \n");
    printf("============================================\n\n");

    /* Step 1: vulnerable kernel writes logs */
    simulate_kernel_activity();

    /* Step 2: attacker reads the same logs */
    attacker_reads_log();

    printf("\n");
    return 0;
}


