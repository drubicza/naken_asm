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

#include "assembler.h"
#include "directives_include.h"
#include "get_tokens.h"
#include "print_error.h"

int add_to_include_path(struct _asm_context *asm_context, char *paths)
{
int ptr = 0;
int n = 0;
char *s;

  s = asm_context->include_path;
  while(!(s[ptr] == 0 && s[ptr+1] == 0)) { ptr++; }
  if (ptr != 0) ptr++;

  while(paths[n] != 0)
  {
    if (paths[n] == ':')
    {
      n++;
      s[ptr++] = 0;
    }
      else
    {
      s[ptr++] = paths[n++];
    }

    if (ptr >= INCLUDE_PATH_LEN-1) return -1;
  }

  return 0;
}

int parse_binfile(struct _asm_context *asm_context)
{
FILE *in;
char token[TOKENLEN];
unsigned char buffer[8192];
//int token_type;
int len;
int n;

  if (asm_context->segment == SEGMENT_BSS)
  {
    printf("Error: .bss segment doesn't support initialized data at %s:%d\n", asm_context->filename, asm_context->line);
    return -1;
  }

  get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
printf("binfile file %s.\n", token);
#endif

  in = fopen(token, "rb");
  if (in == NULL)
  {
    printf("Cannot open binfile file '%s' at %s:%d\n", token, asm_context->filename, asm_context->line);
    return -1;
  }

  while(1)
  {
    len = fread(buffer, 1, 8192, in);
    if (len <= 0) break;

    for (n = 0; n < len; n++)
    {
      memory_write_inc(asm_context, buffer[n], DL_DATA);
    }
    asm_context->data_count += len;
  }

  fclose(in);

  return 0;
}

int parse_include(struct _asm_context *asm_context)
{
char token[TOKENLEN];
//int token_type;
const char *oldname;
int oldline;
FILE *oldfp;
int ret;

  get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
printf("including file %s.\n", token);
#endif

  asm_context->include_count++;

  oldfp = asm_context->in;

  asm_context->in = fopen(token, "rb");

  if (asm_context->in == NULL)
  {
    int ptr = 0;
    char *s = asm_context->include_path;
    char filename[1024];

    while(1)
    {
      if (s[ptr] == 0) break;

      if (strlen(token) + strlen(s+ptr) < 1022)
      {
        sprintf(filename, "%s/%s", s+ptr, token);
#ifdef DEBUG
        printf("Trying %s\n", filename);
#endif
        asm_context->in = fopen(filename, "rb");
        if (asm_context->in != NULL) break;

        if (asm_context->cpu_list_index != -1)
        {
          sprintf(filename, "%s/%s/%s", s+ptr, cpu_list[asm_context->cpu_list_index].name, token);
#ifdef DEBUG
          printf("Trying %s\n", filename);
#endif
          asm_context->in = fopen(filename, "rb");
          if (asm_context->in != NULL) break;
        }
      }

      while (s[ptr] != 0) ptr++;
      ptr++;
    }
  }

  if (asm_context->in == NULL)
  {
    printf("Cannot open include file '%s' at %s:%d\n", token, asm_context->filename, asm_context->line);
    ret = -1;
  }
    else
  {
    oldname = asm_context->filename;
    oldline = asm_context->line;

    asm_context->filename = token;
    asm_context->line = 1;

    ret = assemble(asm_context);

    asm_context->filename = oldname;
    asm_context->line = oldline;
  }

  if (asm_context->in != NULL) { fclose(asm_context->in); }

  asm_context->in = oldfp;
  asm_context->include_count--;

  return ret;
}

