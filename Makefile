################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for 15-441 project 1          #
#                                                                              #
# Author: Matthew Mercedes <mmercede@andrew.cmu.edu>,                          #
#                                                                              #
################################################################################

CC = gcc
CFLAGS = -g -Wall -Werror

all: lisod

log.o: log.c log.h
	$(CC) $(CFLAGS) -c log.c

lisod.o: lisod.c log.h
	$(CC) $(CFLAGS) -c lisod.c

lisod: lisod.o log.o

clean:
	@rm -f lisod.o log.o lisod
