#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



/* 
* Demo: How kernel pointers leaks defeats KASLR
*
* In the real kernel, functions and structs live at
* specific OFFSETS from the kernel base address.
*
* IF we know ANY kernel address, we can calculate
* the base and then find everything else.
*
*/

/* Simulate a kernel struct, in real Android kernel
* something like struct task_struct 
*/


typedef struct {
  int pid;
  char name[16];
  void *next; /* Pointer to next process */
  void *mm; /* Pointer to memory descriptor */


} fake_task_struct;

// simulate what the kernel does internally

void demnstrate_pointer_leak() {
  // Allocate two fake "kernel objects"
  fake_task_struct *task1 = malloc(sizeof(fake_task_struct));
  fake_task_struct *task2 = malloc(sizeof(fake_task_struct));

  task1->pid = 1;
  task1->next = task2; // task1 points to task2
  task2->pid = 2;
  task2->next = NULL;

  printf("\n=== KASLR Demo ===\n\n");

  /* Example of SAFE kernel logging */
  printf("[SAFE]    task->pid   = %d\n", task1->pid);
  printf("[SAFE]    task->name  = init\n\n");

  /* This is what VULNERABLE kernel logging looks like
  * %p prints the actual memory address
  */

  printf("[VULN]    task->next = %p\n", task1->next);
  printf("[VULN]    task->mm   = %p\n", task1->mm);


  printf("\n=== Attack Vector ===\n\n");
  /* Calculating the kernel base address using known offsets */
  uintptr_t leaked_addr = (uintptr_t)task1->next;


  /* In real exploit: kernel_base = leaked_addr - known_offset
  * known_offset comes from reading public kernel symbols
  */
  uintptr_t known_offset = 0x1234560; // example of offset
  uintptr_t kernel_base  = leaked_addr - known_offset;


  printf("  Leaked pointer    : 0x%lx\n", leaked_addr);
  printf("  Known offset      : 0x%lx\n", known_offset);
  printf("  Calculated base   : 0x%lx\n", kernel_base);
  printf("  KASLR defeated    : YES\n\n");

  printf("=== Now attacker can find anything ===\n\n");

  
  /* With kernel base known, attacker calculates addresses
    * of sensitive kernel functions using public symbol table */
  printf("  commit_creds()    : 0x%lx\n", kernel_base + 0x89ab0);
  printf("  prepare_kernel()  : 0x%lx\n", kernel_base + 0x45670);
  printf("  selinux_enforcing : 0x%lx\n", kernel_base + 0xc1230);

  free(task1);
  free(task2);
}


int main() {
    demnstrate_pointer_leak();
    return 0;
}

