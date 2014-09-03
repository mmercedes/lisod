################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for 15-441 project 1          #
#                                                                              #
# Author: Matthew Mercedes <mmercede@andrew.cmu.edu>,                          #
#                                                                              #
################################################################################

default: lisod

lisod:
	@gcc lisod.c -o lisod -Wall -Werror

clean:
	@rm lisod
