/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2013 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "serial.h"

int serial_open(struct _serial *serial, char *device)
{
  serial->fd = open(device, O_RDWR|O_NOCTTY);
  if (serial->fd == -1)
  {
    printf("Couldn't open serial device.\n");
  }
    else
  {
    tcgetattr(serial->fd, &serial->oldtio);

    memset(&serial->newtio, 0, sizeof(struct termios));
    serial->newtio.c_cflag = B9600|CS8|CLOCAL|CREAD;
    serial->newtio.c_iflag = IGNPAR;
    serial->newtio.c_oflag = 0;
    serial->newtio.c_lflag = 0;
    serial->newtio.c_cc[VTIME] = 0;
    serial->newtio.c_cc[VMIN] = 1;

    tcflush(serial->fd, TCIFLUSH);
    tcsetattr(serial->fd, TCSANOW, &serial->newtio);
  }

  return 0;
}

int serial_send(struct _serial *serial, uint8_t *buffer, int len)
{
int i,n;

  i=0;
  while(1)
  {
    n=write(serial->fd, buffer+i, len-i);
    if (n<0) { return -1; }
    i+=n;
  }

  return 0;
}

int serial_readln(struct _serial *serial, char *buffer, int len)
{
int i,n;

  // FIXME - Don't leave this like this
  i=0;
  while(1)
  {
    n=write(serial->fd, buffer+i, 1);
    if (n<0) { return -1; }
    i++;
    if (i==len) { break; }
    if (i=='\n') { break; }
  }

  return i;
}

void serial_close(struct _serial *serial)
{
  if (serial->fd == 0) { return; }

  tcsetattr(serial->fd, TCSANOW, &serial->oldtio);
  close(serial->fd);

  serial->fd = 0;
}

