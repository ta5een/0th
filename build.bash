#!/bin/bash

set -e
set -o pipefail

cc -g -std=c99 -Wall -Wextra -fsanitize=address -o 0c 0c.c
