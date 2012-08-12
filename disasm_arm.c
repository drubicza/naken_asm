/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2012 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm_arm.h"
#include "disasm_common.h"

#define READ_RAM(a) memory_read_m(memory, a)
#define ARM_NIB(n) ((opcode>>n)&0xf)

// NOTE "" is AL
static char *arm_cond[] = 
{
  "eq", "ne", "cs", "ne",
  "mi", "pl", "vs", "vc",
  "hi", "ls" "ge", "lt",
  "gt", "le", "", "nv"
};

char *arm_alu_ops[] = 
{
  "and", "eor", "sub", "rsb",
  "add", "adc", "sbc", "rsc",
  "tst", "teq", "cmp", "cmn",
  "orr", "mov", "bic", "mvn"
};

char *arm_shift[] =
{
  "lsl", "lsr", "asr", "ror"
};

int get_cycle_count_arm(unsigned short int opcode)
{
  return -1;
}

static void arm_calc_shift(char *temp, int shift, int reg)
{
  if ((shift&1)==0)
  {
    sprintf(temp, "r%d, %s r%d", reg, arm_shift[(shift>>1)&0x3], shift>>4);
  }
    else
  {
    sprintf(temp, "r%d, %s #%d", reg, arm_shift[(shift>>1)&0x3], shift>>3);
  }
}

static void arm_register_list(char *instruction, int opcode)
{
char temp[16];
int first=33;
int count=0;
int n;

  for (n=0; n<16; n++)
  {
    if ((opcode&1)==1)
    {
      if (first==33)
      {
        sprintf(temp, "r%d", n);
        if (count!=0) { strcat(instruction, ", "); }
        strcat(instruction, temp);
        first=n;
      }
      count++;
    }
      else
    {
      if (n-first>1)
      {
        sprintf(temp, "-r%d", n-1);
        strcat(instruction, temp);
      }
      first=33;
    }
    opcode>>=1;
  }

}

int disasm_arm(struct _memory *memory, int address, char *instruction, int *cycles_min, int *cycles_max)
{
unsigned int opcode;
char temp[32];

  *cycles_min=1;
  *cycles_max=1;
  opcode=get_opcode32(memory, address);

  if ((opcode&ALU_MASK)==ALU_OPCODE)
  {
    int i=(opcode>>25)&1;
    int s=(opcode>>20)&1;
    int operand2=opcode&0xfff;

    if (i==0)
    {
      arm_calc_shift(temp, operand2>>4, operand2&0xf);
      *cycles_min=2;
      *cycles_max=2;
    }
      else
    {
      int rotate=operand2>>8;
      sprintf(temp, "%d", (operand2&0xff)<<rotate);
    }

    sprintf(instruction, "%s%s%s r%d, r%d, %s", arm_alu_ops[ARM_NIB(28)], arm_cond[ARM_NIB(28)], s==1?"S":"", ARM_NIB(12), ARM_NIB(16), temp);
  }
    else
  if ((opcode&MUL_MASK)==MUL_OPCODE)
  {
    int a=(opcode>>21)&1;
    int s=(opcode>>20)&1;

    if (a==0)
    {
      sprintf(instruction, "mul%s%s r%d, r%d, r%d", arm_cond[ARM_NIB(28)], s==1?"s":"", ARM_NIB(16), ARM_NIB(0), ARM_NIB(8));
    }
      else
    {
      sprintf(instruction, "mla%s%s r%d, r%d, r%d, r%d", arm_cond[ARM_NIB(28)], s==1?"s":"", ARM_NIB(16), ARM_NIB(0), ARM_NIB(8), ARM_NIB(12));
    }
  }
    else
  if ((opcode&MUL_LONG_MASK)==MUL_LONG_OPCODE)
  {
    int u=(opcode>>22)&1;
    int a=(opcode>>21)&1;
    int s=(opcode>>20)&1;

    if (a==0)
    {
      sprintf(instruction, "%cmull%s%s r%d, r%d, r%d", u==1?'u':'s', arm_cond[ARM_NIB(28)], s==1?"s":"", ARM_NIB(16), ARM_NIB(0), ARM_NIB(8));
    }
      else
    {
      sprintf(instruction, "%cmlal%s%s r%d, r%d, r%d, r%d", u==1?'u':'s', arm_cond[ARM_NIB(28)], s==1?"s":"", ARM_NIB(16), ARM_NIB(0), ARM_NIB(8), ARM_NIB(12));
    }
  }
    else
  if ((opcode&SWAP_MASK)==SWAP_OPCODE)
  {
    int b=(opcode>>22)&1;

    sprintf(instruction, "swap%s%s r%d, r%d, [r%d]", arm_cond[ARM_NIB(28)], b==1?"b":"", ARM_NIB(12), ARM_NIB(0), ARM_NIB(16));
  }
    else
  if ((opcode&MRS_MASK)==MRS_OPCODE)
  {
    int ps=(opcode>>22)&1;

    sprintf(instruction, "mrs%s r%d, %s", arm_cond[ARM_NIB(28)], ARM_NIB(12), ps==1?"SPSR":"CPSR");
  }
    else
  if ((opcode&MSR_ALL_MASK)==MSR_ALL_OPCODE)
  {
    int ps=(opcode>>22)&1;

    sprintf(instruction, "msr%s %s, r%d", arm_cond[ARM_NIB(28)], ps==1?"SPSR":"CPSR", ARM_NIB(0));
  }
    else
  if ((opcode&MSR_FLAG_MASK)==MSR_FLAG_OPCODE)
  {
    int i=(opcode>>25)&1;
    int ps=(opcode>>22)&1;

    if (i==0)
    {
      sprintf(instruction, "msr%s %s_flg, r%d", arm_cond[ARM_NIB(28)], ps==1?"SPSR":"CPSR", ARM_NIB(0));
    }
      else
    {
      sprintf(instruction, "msr%s %s_flg, #%d", arm_cond[ARM_NIB(28)], ps==1?"SPSR":"CPSR", (opcode&0xff)<<ARM_NIB(8));
    }
  }
    else
  if ((opcode&LDR_STR_MASK)==LDR_STR_OPCODE)
  {
    int ls=(opcode>>20)&1;
    int w=(opcode>>21)&1;
    int b=(opcode>>22)&1;
    int u=(opcode>>23)&1;
    int pr=(opcode>>24)&1;
    int i=(opcode>>25)&1;
    int offset=opcode&0xfff;
    int rn=ARM_NIB(16);

    if (i==0)
    {
      sprintf(temp, "r%d, #%s%d", rn, u==0?"-":"", offset);
    }
      else
    {
      int shift=offset>>8;
      int type=(shift>>1)&0x3;
      int rm=offset&0xf;

      if ((shift&1)==0)
      {
        *cycles_min=2;
        *cycles_max=2;

        if (pr==1)
        {
          sprintf(temp, "[r%d, r%d, %s r%d]", rn, rm, arm_shift[type], shift>>4);
        }
          else
        {
          sprintf(temp, "[r%d], r%d, %s r%d", rn, rm, arm_shift[type], shift>>4);
        }
      }
        else
      {
        if (pr==1)
        {
          sprintf(temp, "[r%d, r%d, %s #%d]", rn, rm, arm_shift[type], shift>>3);
        }
          else
        {
          sprintf(temp, "[r%d], r%d, %s #%d", rn, rm, arm_shift[type], shift>>3);
        }
      }
    }

    sprintf(instruction, "%s%s%s r%d, %s%s", ls==0?"str":"ldr", arm_cond[ARM_NIB(28)], b==0?"":"b", ARM_NIB(12), temp, w==0?"":"!");
  }
    else
  if ((opcode&UNDEF_MASK)==UNDEF_OPCODE)
  {
    strcpy(instruction, "undefined");
  }
    else
  if ((opcode&LDM_STM_MASK)==LDM_STM_OPCODE)
  {
    char *pru_str[] = { "db", "ib", "da", "ia" };
    int ls=(opcode>>20)&1;
    int w=(opcode>>21)&1;
    int s=(opcode>>22)&1;
    int pru=(opcode>>23)&0x3;

    sprintf(instruction, "%s%s%s r%d%s, {", ls==1?"ldm":"stm", pru_str[pru], s==1?"s":"",  ARM_NIB(16), w==1?"!":"");

    arm_register_list(instruction, opcode);

    strcat(instruction, "}");
  }
    else
  if ((opcode&BRANCH_MASK)==BRANCH_OPCODE)
  {
    int l=(opcode>>24)&1;

    sprintf(temp, "%s%s 0x%02x", l==0?"b":"bl", arm_cond[ARM_NIB(28)], (opcode&0xffffff)<<2);

    *cycles_max=3;
  }
    else
  if ((opcode&BRANCH_EXCH_MASK)==BRANCH_EXCH_OPCODE)
  {
    // FIXME - implement
    printf("branch exch?\n");
  }
    else
  if ((opcode&CO_TRANSFER_MASK)==CO_TRANSFER_OPCODE)
  {
    int ls=(opcode>>20)&1;
    int w=(opcode>>21)&1;
    int n=(opcode>>22)&1;
    int u=(opcode>>23)&1;
    int pr=(opcode>>24)&1;
    int offset=opcode&0xff;

    if (offset==0)
    {
      sprintf(instruction, "%s%s%s %d, cr%d, [r%d]", ls==1?"ldc":"stc", arm_cond[ARM_NIB(28)], n==1?"l":"", ARM_NIB(8), ARM_NIB(12), ARM_NIB(16));
    }
      else
    if (pr==1)
    {
      sprintf(instruction, "%s%s%s %d, cr%d, [r%d, #%s%d]%s", ls==1?"ldc":"stc", arm_cond[ARM_NIB(28)], n==1?"l":"", ARM_NIB(8), ARM_NIB(12), ARM_NIB(16), u==0?"-":"", offset, w==1?"!":"");
    }
      else
    {
      sprintf(instruction, "%s%s%s %d, cr%d, [r%d], #%s%d%s", ls==1?"ldc":"stc", arm_cond[ARM_NIB(28)], n==1?"l":"", ARM_NIB(8), ARM_NIB(12), ARM_NIB(16), u==0?"-":"", offset, w==1?"!":"");
    }
  }
    else
  if ((opcode&CO_OP_MASK)==CO_OP_OPCODE)
  {
    sprintf(temp, "cdp%s %d, %d, cr%d, cr%d, cr%d, %d", arm_cond[ARM_NIB(28)], ARM_NIB(8), ARM_NIB(20), ARM_NIB(12), ARM_NIB(16), ARM_NIB(0), (opcode>>5)&0x7);
  }
    else
  if ((opcode&CO_RTRANSFER_MASK)==CO_RTRANSFER_OPCODE)
  {
    // FIXME - implement this
  }
    else
  if ((opcode&CO_SWI_MASK)==CO_SWI_OPCODE)
  {
    strcpy(instruction, "swi");
  }
    else
  {
    printf("Internal Error: Unknown ARM opcode %08x, %s:%d\n", opcode, __FILE__, __LINE__);
    strcpy(instruction, "???");
  }

  return 4;
}

void list_output_arm(struct _asm_context *asm_context, int address)
{
int cycles_min,cycles_max,count;
char instruction[128];
unsigned int opcode=get_opcode32(&asm_context->memory, address);

  fprintf(asm_context->list, "\n");
  count=disasm_arm(&asm_context->memory, address, instruction, &cycles_min, &cycles_max);
  fprintf(asm_context->list, "0x%04x: 0x%08x %-40s cycles: ", address, opcode, instruction);

  if (cycles_min==cycles_max)
  { fprintf(asm_context->list, "%d\n", cycles_min); }
    else
  { fprintf(asm_context->list, "%d-%d\n", cycles_min, cycles_max); }

}

void disasm_range_arm(struct _memory *memory, int start, int end)
{
// Are these correct and the same for all MSP430's?
char *vectors[16] = { "", "", "", "", "", "",
                      "", "", "", "",
                      "", "", "", "",
                      "",
                      "Reset/Watchdog/Flash" };
char instruction[128];
int vectors_flag=0;
int cycles_min=0,cycles_max=0;
int num;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start<=end)
  {
    if (start>=0xffe0 && vectors_flag==0)
    {
      printf("Vectors:\n");
      vectors_flag=1;
    }

    num=READ_RAM(start)|(READ_RAM(start+1)<<8);

    disasm_arm(memory, start, instruction, &cycles_min, &cycles_max);

    if (vectors_flag==1)
    {
      printf("0x%04x: 0x%04x  Vector %2d {%s}\n", start, num, (start-0xffe0)/2, vectors[(start-0xffe0)/2]);
      start+=2;
      continue;
    }

    if (cycles_min<1)
    {
      printf("0x%04x: 0x%04x %-40s ?\n", start, num, instruction);
    }
      else
    if (cycles_min==cycles_max)
    {
      printf("0x%04x: 0x%04x %-40s %d\n", start, num, instruction, cycles_min);
    }
      else
    {
      printf("0x%04x: 0x%04x %-40s %d-%d\n", start, num, instruction, cycles_min, cycles_max);
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
