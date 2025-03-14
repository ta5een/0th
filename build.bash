#!/bin/bash

set -xeo pipefail

cc \
  -g \
  -std=c99 \
  -pedantic-errors \
  -Wall \
  -Werror \
  -Wextra \
  -fsanitize=address \
  -o 0th \
  0th.c
