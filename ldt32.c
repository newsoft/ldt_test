/*
 * Sample code for exercising __NR_modify_ldt syscall.
 *
 * Compile with:
 * gcc -m32 -o ldt32 ldt32.c
 *
 * Note: to compile 32-bit code on Ubuntu 64-bit, make sure 'gcc-multilib'
 * package has been installed.
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
int cookie = 0x1234578;


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


int read_value() {
  int value;

  /*
   * Segment selector format:
   * 15..3 index
   *     2 table indicator (0=GDT, 1=LDT)
   *  1..0 RPL
   */

  asm ("pushl %%es\n\t"
      "movl $0x7, %%eax\n\t"  // Segment: index=0000000000000 table=1 rpl=11 (3).
      "movl %%eax, %%es\n\t"
      "movl %%es:0, %0\n\t"   // Read cookie value through ES:[0].
      "popl %%es\n\t"
      : "=r" (value)  // Output operands.
      :               // Input operands.
      : "cc", "eax"   // Clobbers flags and EAX.
      );

  return value;
}


int main() {

  printf("Value (direct) = 0x%08x\n", cookie);
  setup_ldt();
  printf("Value (es:[0]) = 0x%08x\n", read_value());

  return EXIT_SUCCESS;
}
