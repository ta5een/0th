#!/bin/bash

set -xeo pipefail

cc \
  -g \
  -std=c99 \
  -pedantic-errors \
  -Wall \
  -Werror \
  -Wextra \
  -Wcast-align \
  -Wcast-qual \
  -Wconversion \
  -Wdouble-promotion \
  -Wformat=2 \
  -Wmissing-prototypes \
  -Wshadow \
  -fsanitize=undefined,address \
  -o 0th \
  0th.c
