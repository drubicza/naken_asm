/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2014 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm_common.h"
#include "disasm_stm8.h"
#include "table_stm8.h"

#define READ_RAM(a) memory_read_m(memory, a)
#define READ_RAM16(a) ((memory_read_m(memory, a)<<8)|(memory_read_m(memory, a+1)))
#define READ_RAM24(a) ((memory_read_m(memory, a)<<16)|(memory_read_m(memory, a+1)<<8)|(memory_read_m(memory, a+2)))

#define SINGLE_OPCODE(pre, op, cycles, size, instr) \
  if (opcode==op && prefix==pre) \
  { \
    strcpy(instruction, instr); \
    *cycles_min=cycles; \
    *cycles_max=cycles; \
    return size; \
  }

int get_cycle_count_stm8(unsigned short int opcode)
{
  return -1;
}

int disasm_stm8(struct _memory *memory, int address, char *instruction, int *cycles_min, int *cycles_max)
{
unsigned char opcode;
//int function;
int n;
//char temp[32];
int prefix=0;

  instruction[0]=0;

  *cycles_min=-1;
  *cycles_max=-1;

  opcode=READ_RAM(address);

  if (opcode==0x90 || opcode==0x91 || opcode==0x92 || opcode==0x72)
  {
    prefix=opcode;
    address++;
    opcode=READ_RAM(address);
  }

  if (prefix==0x00)
  {
    n=0;
    while(stm8_single[n].instr!=NULL)
    {
      if (stm8_single[n].opcode==opcode)
      {
        strcpy(instruction, stm8_single[n].instr);
        *cycles_min=stm8_single[n].cycles;
        *cycles_max=stm8_single[n].cycles;
        return 1;
      }

      n++;
    }
  }

  // Check for ldf
  {
    int size=4;
    int cycles=1;
    unsigned char opcodes[] =
    {
      0xbc, 0xaf, 0xbd, 0xa7
    };

    for (n=0; n<4; n++)
    {
      if (opcode==opcodes[n])
      {
        char temp[32];

        if ((n&1)==0)
        {
          if (prefix==0x00)
          {
            sprintf(temp, "$%06x", READ_RAM24(address+1));
          }
            else
          if (prefix==0x92)
          {
            sprintf(temp, "[$%04x]", READ_RAM16(address+1));
            cycles=5;
          }
            else
          { break; }
        }
          else
        {
          if (prefix==0x00)
          {
            sprintf(temp, "($%06x, X)", READ_RAM24(address+1));
          }
            else
          if (prefix==0x90)
          {
            sprintf(temp, "($%06x, Y)", READ_RAM24(address+1));
            size=5;
          }
            else
          if (prefix==0x92)
          {
            sprintf(temp, "([$%04x], X)", READ_RAM16(address+1));
            cycles=5;
          }
            else
          if (prefix==0x91)
          {
            sprintf(temp, "([$%04x], Y)", READ_RAM16(address+1));
            cycles=5;
          }
            else
          { break; }
        }

        if (n<2) { sprintf(instruction,"ldf A, %s", temp); }
        else { sprintf(instruction,"ldf %s, A", temp); cycles=(cycles==5)?4:1; }
        *cycles_min=cycles;
        *cycles_max=cycles;
        return size;
      }
    }
  }

  n=0;
  while(stm8_x_y[n].instr!=NULL)
  {
    if (stm8_x_y[n].opcode==opcode)
    {
      if (prefix==0x00)
      {
        sprintf(instruction, "%s X", stm8_x_y[n].instr);
      }
        else
      if (prefix==0x90)
      {
        sprintf(instruction, "%s Y", stm8_x_y[n].instr);
      }
        else
      {
        break;
      }

      *cycles_min=stm8_x_y[n].cycles;
      *cycles_max=stm8_x_y[n].cycles;
      return 1;
    }

    n++;
  }

  if (opcode==0xad && prefix==0x00)
  {
    char offset=(char)READ_RAM(address+1);
    sprintf(instruction, "callr $%04x (%d)", address+2+offset, offset);
    *cycles_min=4;
    *cycles_max=4;
    return 3;
  }

  if (opcode==0x8d || opcode==0xac)
  {
    char *instr=(opcode==0x8d)?"callf":"jpf";
    if (prefix==0x00)
    {
      sprintf(instruction, "%s $%06x", instr, READ_RAM24(address+1));
      *cycles_min=(opcode==0x8d)?5:2;
      *cycles_max=*cycles_min;;
      return 4;
    }
      else
    if (prefix==0x92)
    {
      sprintf(instruction, "%s [$%04x]", instr, READ_RAM16(address+1));
      *cycles_min=(opcode==0x8d)?8:6;
      *cycles_max=*cycles_min;;
      return 4;
    }
  }

  // Check for addw, subw
  {
    unsigned char opcodes[] =
    {
      0x1c, 0xbb, 0xfb, 0xa9, 0xb9, 0xf9,
      0x1d, 0xb0, 0xf0, 0xa2, 0xb2, 0xf2,
    };

    for (n=0; n<12; n++)
    {
      if (opcode==opcodes[n])
      {
        if ((n%6)==0 && prefix!=0) { continue; }
        if ((n%6)!=0 && prefix!=0x72) { continue; }
        int size;
        *cycles_min=2;
        *cycles_max=2;
        char *instr=(n>=6)?"subw":"addw";
        char *reg=(((n/3)&1)==0)?"X":"Y";
        int v=n%3;
        size=0; // Good job bitching about nothing clang
        if (v==0)
        { sprintf(instruction, "%s %s, #$%04x", instr, reg, READ_RAM16(address+1)); size=3; }
        else if (v==1)
        { sprintf(instruction, "%s %s, $%04x", instr, reg, READ_RAM16(address+1)); size=4; }
        else if (v==2)
        { sprintf(instruction, "%s %s, ($%02x,SP)", instr, reg, READ_RAM(address+1)); size=4; }
        return size;
      }
    }
  }

  int opcode_nibble=opcode&0x0f;
  if (stm8_type1[opcode_nibble]!=NULL)
  {
    int masked=opcode&0xf0;
    char operand[32];
    operand[0]=0;
    int cycles=1;
    int size=2;

    if (opcode==0xae && prefix==0)
    {
      sprintf(operand, "#$%04x", READ_RAM16(address+1));
      size++;
    }
      else
    if (opcode==0x7b && prefix==0)
    {
      sprintf(operand, "($%02x, SP)", READ_RAM(address+1));
    }
      else
    if (opcode==0x6b && prefix==0)
    {
      sprintf(operand, "($%02x, SP)", READ_RAM(address+1));
    }
      else
    if (masked==0x10 && prefix==0)
    {
      sprintf(operand, "($%02x, SP)", READ_RAM(address+1));
    }
      else
    if (masked==0xa0 && prefix==0)
    {
      sprintf(operand, "#$%02x", READ_RAM(address+1));
    }
      else
    if (masked==0xb0)
    {
      sprintf(operand, "$%02x", READ_RAM(address+1));
    }
      else
    if (masked==0xc0)
    {
      if (prefix==0)
      { sprintf(operand, "$%04x", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x92)
      { sprintf(operand, "[$%02x]", READ_RAM(address+1)); }
        else
      if (prefix==0x72)
      { sprintf(operand, "[$%04x]", READ_RAM16(address+1)); size++; }

      if (prefix!=0) { cycles=4; }
    }
      else
    if (masked==0xf0)
    {
      if (prefix==0) { strcpy(operand, "(X)"); }
      else if (prefix==0x90) { strcpy(operand, "(Y)"); }
      size=1;
    }
      else
    if (masked==0xe0)
    {
      if (prefix==0)
      { sprintf(operand, "($%02x,X)", READ_RAM(address+1)); }
        else
      if (prefix==0x90)
      { sprintf(operand, "($%02x,Y)", READ_RAM(address+1)); }
    }
      else
    if (masked==0xd0)
    {
      if (prefix==0)
      { sprintf(operand, "($%04x,X)", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x90)
      { sprintf(operand, "($%04x,Y)", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x92)
      { sprintf(operand, "([$%02x],X)", READ_RAM(address+1)); }
        else
      if (prefix==0x72)
      { sprintf(operand, "([$%04x],X)", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x91)
      { sprintf(operand, "([$%02x],Y)", READ_RAM(address+1)); }

      if (prefix!=0 && prefix!=0x90) { cycles=4; }
    }

    if (operand[0]!=0)
    {
      if (opcode_nibble==0x0e) { cycles=(prefix==0x00)?2:5; }
        else
      if (opcode_nibble==0x0d) { cycles=(cycles==1)?4:6; }
        else
      if (opcode_nibble==0x0c)
      {
        if (cycles==4) { cycles=5; }
        else if (prefix==0x90 && (opcode==0xec || opcode==0xdc)) { cycles=2; }
      }

      *cycles_min=cycles;
      *cycles_max=cycles;
      if (prefix!=0) { size++; }

      if (opcode_nibble==7)
      { sprintf(instruction, "%s %s, A", stm8_type1[opcode_nibble], operand); }
        else
      if (opcode_nibble==3)
      { sprintf(instruction, "%s X, %s", stm8_type1[opcode_nibble], operand); }
        else
      if (opcode_nibble==0xd || opcode_nibble==0xc)
      { sprintf(instruction, "%s %s", stm8_type1[opcode_nibble], operand); }
        else
      { sprintf(instruction, "%s A, %s", stm8_type1[opcode_nibble], operand); }

      return size;
    }
  }

  if (stm8_type2[opcode_nibble]!=NULL)
  {
    int masked=opcode&0xf0;
    char operand[32];
    operand[0]=0;
    int cycles=1;
    int size=2;

    if (masked==0x00 && prefix==0)
    {
      sprintf(operand, "($%02x,SP)", READ_RAM(address+1));
    }
      else
    if (masked==0x50 && prefix==0x72)
    {
      sprintf(operand, "$%04x", READ_RAM16(address+1));
      size=3;
    }
      else
    if (masked==0x30)
    {
      if (prefix==0)
      { sprintf(operand, "$%02x", READ_RAM(address+1)); }
        else
      if (prefix==0x92)
      { sprintf(operand, "[$%02x]", READ_RAM(address+1)); cycles=4; }
        else
      if (prefix==0x72)
      { sprintf(operand, "[$%04x]", READ_RAM16(address+1)); size++; cycles=4; }
    }
      else
    if (masked==0x70)
    {
      if (prefix==0) { strcpy(operand, "(X)"); }
      else if (prefix==0x90) { strcpy(operand, "(Y)"); }
      size=1;
    }
      else
    if (masked==0x40)
    {
      if (prefix==0) { strcpy(operand, "A"); size=1; }
        else
      if (prefix==0x72)
      { sprintf(operand, "($%04x,X)", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x90)
      { sprintf(operand, "($%04x,Y)", READ_RAM16(address+1)); size++; }
    }
      else
    if (masked==0x60)
    {
      if (prefix==0)
      { sprintf(operand, "($%02x,X)", READ_RAM(address+1)); }
        else
      if (prefix==0x90)
      { sprintf(operand, "($%02x,Y)", READ_RAM(address+1)); }
        else
      if (prefix==0x92)
      { sprintf(operand, "([$%02x],X)", READ_RAM(address+1)); }
        else
      if (prefix==0x72)
      { sprintf(operand, "([$%04x],X)", READ_RAM16(address+1)); size++; }
        else
      if (prefix==0x91)
      { sprintf(operand, "([$%02x],Y)", READ_RAM(address+1)); }

      if (prefix!=0 && prefix!=0x90) { cycles=4; }
    }

    if (operand[0]!=0)
    {
      *cycles_min=cycles;
      *cycles_max=cycles;
      if (prefix!=0) { size++; }

      sprintf(instruction, "%s %s", stm8_type2[opcode_nibble], operand);

      return size;
    }
  }

  if ((prefix==0x90 && (opcode>>4)==1) || (prefix==0x72 && (opcode>>4)<=1))
  {
    //int bit_oper=((opcode&16)>>3)|(opcode&1);

    if (prefix==0x72 && (opcode>>4)==0)
    {
      sprintf(instruction, "%s $%04x, #%d, $%04x", stm8_bit_oper[(opcode&1)+4], READ_RAM16(address+1), (opcode&0xf)>>1, (address+4)+((char)READ_RAM(address+4)));
      *cycles_min=2;
      *cycles_max=3;
      return 5;
    }
      else
    {
      sprintf(instruction, "%s $%04x, #%d", stm8_bit_oper[(opcode&1)+((prefix==0x72)?0:2)], READ_RAM16(address+1), (opcode&0xf)>>1);
      *cycles_min=1;
      *cycles_max=1;
      return 4;
    }
  }

  n=0;
  while(stm8_r_r[n].instr!=NULL)
  {
    if (stm8_r_r[n].opcode==opcode)
    {
      *cycles_min=stm8_r_r[n].cycles_min;
      *cycles_max=stm8_r_r[n].cycles_max;

      if (n<2)
      {
        sprintf(instruction, "%s %c,A", stm8_r_r[n].instr, (prefix==0x90)?'Y':'X');
        if (prefix==0x90) return 2;
      }
        else
      {
        sprintf(instruction, "%s X,Y", stm8_r_r[n].instr);
      }

      return 1;
    }

    n++;
  }

  if (opcode>=0x20 && opcode <=0x2f)
  {
    n=0;
    while(stm8_jumps[n].instr!=NULL)
    {
      if (stm8_jumps[n].opcode==opcode && stm8_jumps[n].prefix==prefix)
      {
        char offset=(char)READ_RAM(address+1);
        sprintf(instruction, "%s $%04x (%d)", stm8_jumps[n].instr, address+2+offset, offset);
        *cycles_min=1;
        *cycles_max=2;

        if (n==0 || n==1) { *cycles_min=2; }
        else if (n==2) { *cycles_max=1; }

        return (prefix==0)?2:3;
      }

      n++;
    }
  }

  SINGLE_OPCODE(0x72, 0x8f, 1, 2, "wfe")
  SINGLE_OPCODE(0x00, 0x84, 1, 1, "pop A")
  SINGLE_OPCODE(0x00, 0x86, 1, 1, "pop CC")
  SINGLE_OPCODE(0x00, 0x88, 1, 1, "push A")
  SINGLE_OPCODE(0x00, 0x8a, 1, 1, "push CC")
  SINGLE_OPCODE(0x00, 0x41, 1, 1, "exg A, XL")
  SINGLE_OPCODE(0x00, 0x61, 1, 1, "exg A, YL")
  SINGLE_OPCODE(0x00, 0xff, 2, 1, "ldw (X), Y")

  if (prefix==0x00)
  {
    if (opcode==0x32 && prefix==0x00)
    {
      sprintf(instruction, "pop $%04x", READ_RAM16(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 3;
    }

    if (opcode==0x4b)
    {
      sprintf(instruction, "push #$%02x", READ_RAM(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 2;
    }

    if (opcode==0x3b)
    {
      sprintf(instruction, "push $%04x", READ_RAM16(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 3;
    }

    if (opcode==0x31)
    {
      sprintf(instruction, "exg A, $%04x", READ_RAM16(address+1));
      *cycles_min=3;
      *cycles_max=3;
      return 3;
    }

    if (opcode==0x35)
    {
      sprintf(instruction, "mov $%04x, #$%02x", READ_RAM16(address+2), READ_RAM(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 4;
    }

    if (opcode==0x45)
    {
      sprintf(instruction, "mov $%02x, $%02x", READ_RAM(address+2), READ_RAM(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 3;
    }

    if (opcode==0x55)
    {
      sprintf(instruction, "mov $%04x, $%04x", READ_RAM16(address+3), READ_RAM16(address+1));
      *cycles_min=1;
      *cycles_max=1;
      return 5;
    }

    if (opcode==0x5b)
    {
      sprintf(instruction, "addw SP, #$%02x", READ_RAM(address+1));
      *cycles_min=2;
      *cycles_max=2;
      return 2;
    }

    if (opcode==0xbf)
    {
      sprintf(instruction, "ldw $%02x, X", READ_RAM(address+1));
      *cycles_min=2;
      *cycles_max=2;
      return 2;
    }

    if (opcode==0xcf)
    {
      sprintf(instruction, "ldw $%04x, X", READ_RAM16(address+1));
      *cycles_min=2;
      *cycles_max=2;
      return 3;
    }

    if (opcode==0xef)
    {
      sprintf(instruction, "ldw ($%02x,X), Y", READ_RAM(address+1));
      *cycles_min=2;
      *cycles_max=2;
      return 2;
    }

    if (opcode==0xdf)
    {
      sprintf(instruction, "ldw ($%04x,X), Y", READ_RAM16(address+1));
      *cycles_min=2;
      *cycles_max=2;
      return 3;
    }
  }


  strcpy(instruction, "???");

  return 1;
}

void list_output_stm8(struct _asm_context *asm_context, int address)
{
int cycles_min,cycles_max,count;
char instruction[128];
//unsigned int opcode=READ_RAM(&asm_context->memory, address);
//unsigned int opcode=0;
int n;

  fprintf(asm_context->list, "\n");
  count=disasm_stm8(&asm_context->memory, address, instruction, &cycles_min, &cycles_max);
  fprintf(asm_context->list, "0x%04x:", address);

  for (n = 0; n < 5; n++)
  {
    if (n < count)
    {
      fprintf(asm_context->list, " %02x", memory_read_m(&asm_context->memory, address+n));
    }
      else
    {
      fprintf(asm_context->list, "   ");
    }
  }
  fprintf(asm_context->list, " %-40s cycles: ", instruction);

  if (cycles_min==cycles_max)
  { fprintf(asm_context->list, "%d\n", cycles_min); }
    else
  { fprintf(asm_context->list, "%d-%d\n", cycles_min, cycles_max); }

}

void disasm_range_stm8(struct _memory *memory, int start, int end)
{
char instruction[128];
int cycles_min=0,cycles_max=0;
int num;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start<=end)
  {
    num=READ_RAM(start)|(READ_RAM(start+1)<<8);

    disasm_stm8(memory, start, instruction, &cycles_min, &cycles_max);

    if (cycles_min<1)
    {
      printf("0x%04x: 0x%08x %-40s ?\n", start, num, instruction);
    }
      else
    if (cycles_min==cycles_max)
    {
      printf("0x%04x: 0x%08x %-40s %d\n", start, num, instruction, cycles_min);
    }
      else
    {
      printf("0x%04x: 0x%08x %-40s %d-%d\n", start, num, instruction, cycles_min, cycles_max);
    }

#if 0
    count-=4;
    while (count>0)
    {
      start=start+4;
      num=READ_RAM(start)|(READ_RAM(start+1)<<8);
      printf("0x%04x: 0x%04x\n", start, num);
      count-=4;
    }
#endif

    start=start+4;
  }
}

