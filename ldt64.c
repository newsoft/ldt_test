/*
 * Sample code for exercising __NR_modify_ldt syscall.
 *
 * Compile with:
 * gcc -m64 -o ldt64 ldt64.c
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <asm/ldt.h>

/* 
 * Implementation weirdness:
 * 0 is LDT_READ
 * 1 is LDT_WRITE
 * 2 returns all zeroes
 * 0x11 is an alias for LDT_WRITE
 */

#define LDT_READ  0
#define LDT_WRITE 1


// Global var that will be accessed through unusual segment (ES:[0]).
long cookie = 0x1234578;


int setup_ldt() {
  int ret;
  struct user_desc* table_entry_ptr = NULL;

  // Allocates memory for a user_desc struct.
  table_entry_ptr = (struct user_desc*)malloc(sizeof(struct user_desc));

  // Fills the user_desc struct.
  table_entry_ptr->entry_number = 0;
  table_entry_ptr->base_addr = (long)&cookie;  // Unclean pointer conversion.
  table_entry_ptr->limit = 0xffffffff >> 12;
  table_entry_ptr->seg_32bit = 0x1;            // 16-bit segment or not.
  table_entry_ptr->contents = 0x0;             // Direction + Conforming bits.
  table_entry_ptr->read_exec_only = 0x0;
  table_entry_ptr->limit_in_pages = 0x0;       // Granularity (byte or page).
  table_entry_ptr->seg_not_present = 0x0;
  table_entry_ptr->useable = 0x1;

  // Writes a user_desc struct to the LDT.
  ret = syscall( __NR_modify_ldt,
    LDT_WRITE,
    table_entry_ptr,
    sizeof(struct user_desc)
  );

  free(table_entry_ptr);

  return ret;  // Returns 0 on success.
}


long read_value() {
  long value;

  /*
   * Segment selector format:
   * 15..3 index
   *     2 table indicator (0=GDT, 1=LDT)
   *  1..0 RPL
   */

  /* 
   * 64-bit architecture mostly dropped segment registers.
   * Only FS and GS can be pushed/poped.
   * FS is already used by glibc for Thread Local Storage.
   * GS is free for use.
   */

  asm ("pushq %%gs\n\t"
      "movq $0x7, %%rax\n\t"  // Segment: index=0000000000000 table=1 rpl=11 (3).
      "movq %%rax, %%gs\n\t"
      "movq %%gs:0, %0\n\t"   // Read cookie value through GS:[0].
      "popq %%gs\n\t"
      : "=r" (value)  // Output operands.
      :               // Input operands.
      : "cc", "rax"   // Clobbers flags and RAX.
      );

  return value;
}


int main() {

  printf("Value (direct) = 0x%016lx\n", cookie);
  setup_ldt();
  printf("Value (gs:[0]) = 0x%016lx\n", read_value());

  return EXIT_SUCCESS;
}
