#!/bin/bash

set -eo pipefail

PASS=0
FAIL=0

test_case() {
  local name="$1"
  local input="$2"
  local expected="$3"

  output=$(echo "$input" | ./0th 2>&1) || true
  if echo "$output" | grep -q "$expected"; then
    echo "PASS: $name"
    PASS=$((PASS + 1))
  else
    echo "FAIL: $name"
    echo "  input:    $input"
    echo "  expected: $expected"
    echo "  got:      $output"
    FAIL=$((FAIL + 1))
  fi
}

# Empty input
test_case "empty input" "" "<0> (empty)"

# Integer literals - decimal
test_case "decimal positive" "42" "0x0*2A"
test_case "decimal negative" "-1" "0xF\+"  # -1 is all Fs in two's complement

# Integer literals - bases
test_case "hex literal" "16#ff" "0x0*FF"
test_case "binary literal" "2#1010" "0x0*A"
test_case "octal literal" "8#77" "0x0*3F"

# Integer literals - negative with base
test_case "negative hex" "-16#1" "0xF\+"  # -1 is all Fs

# Stack operations
test_case "multiple values" "1 2 3" "<3>.*0x0*1.*0x0*2.*0x0*3"
test_case "drop" "1 2 drop" "<1>.*0x0*1"
test_case "dup" "42 dup" "<2>.*0x0*2A"
test_case "swap" "1 2 swap" "<2>.*0x0*2.*0x0*1"

# Arithmetic
test_case "addition" "1 2 +" "<1>.*0x0*3"
test_case "addition hex" "16#10 16#20 +" "0x0*30"

# Edge cases
test_case "leading zeros in base" "08#7" "0x0*7"
test_case "base 36" "36#z" "0x0*23"  # z = 35 = 0x23

# Undefined word
test_case "undefined word" "notaword" "VM_ERROR_UNDEFINED_WORD"

# Summary
echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
