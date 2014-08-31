################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for 15-441 project 1          #
#                                                                              #
# Author: Matthew Mercedes <mmercede@andrew.cmu.edu>,                          #
#                                                                              #
################################################################################

default: echo_server

echo_server:
	@gcc echo_server.c -o echo_server -Wall -Werror

clean:
	@rm echo_server
