// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "demo_system.h"

#define FP_EXP 12
#define FP_MANT 15
#define MAKE_FP(i, f, f_bits) ((i << FP_EXP) | (f << (FP_EXP - f_bits)))

typedef uint32_t cmplx_packed_t;

static inline cmplx_packed_t cmplx_mul_insn(uint32_t a, uint32_t b) {
  uint32_t result;

  asm (".insn r CUSTOM_0, 0, 0, %0, %1, %2" :
       "=r"(result) :
       "r"(a), "r"(b));

  return result;
}

static inline cmplx_packed_t cmplx_add_insn(uint32_t a, uint32_t b) {
  uint32_t result;

  asm (".insn r CUSTOM_0, 1, 0, %0, %1, %2" :
       "=r"(result) :
       "r"(a), "r"(b));

  return result;
}

static inline int32_t cmplx_abs_sq_insn(uint32_t a) {
  int32_t result;

  asm (".insn r CUSTOM_0, 2, 0, %0, %1, x0" :
       "=r"(result) :
       "r"(a));

  return result;
}

typedef struct {
  int32_t real;
  int32_t imag;
} cmplx_t;

cmplx_t test_nums[] = {
  {MAKE_FP(1, 0, 0), 0},
  {0, MAKE_FP(1, 0, 0)},

  {MAKE_FP(2, 0, 0), MAKE_FP(1, 0, 0)},
  {MAKE_FP(1, 0, 0), MAKE_FP(2, 0, 0)},

  {MAKE_FP(2, 0, 0), MAKE_FP(1, 0, 0)},
  {-MAKE_FP(1, 0, 0), -MAKE_FP(2, 0, 0)},

  {MAKE_FP(4, 0, 0), -MAKE_FP(1, 0, 0)},
  {MAKE_FP(1, 0, 0), MAKE_FP(0, 1, 1)},

  {MAKE_FP(7, 0xFFF, 12), -MAKE_FP(8, 0x0, 0)},
  {-MAKE_FP(8, 0x0, 0), MAKE_FP(7, 0xFFF, 12)},

  {MAKE_FP(7, 0xFFF, 12), 0},
  {0, -MAKE_FP(7, 0xFFF, 12)},

  {-MAKE_FP(3, 0x123, 12), MAKE_FP(2, 0x1b, 5)},
  {-MAKE_FP(1, 0x2d, 6), -MAKE_FP(0, 0x87, 8)},

  {MAKE_FP(4, 0, 0), MAKE_FP(4, 0, 8)},
  {-MAKE_FP(2, 0, 0), MAKE_FP(2, 0, 0)},

  {MAKE_FP(4, 0xD8F, 12), -MAKE_FP(0, 0xff, 8)},
  {-MAKE_FP(2, 0x6c, 7), MAKE_FP(3, 0x3a5, 10)}
};

#define NUM_TESTS 9

int32_t fp_clamp(int32_t x) {
  if ((x < 0) && (x < -(1 << FP_MANT))) {
      return -(1 << FP_MANT);
  }

  if ((x > 0) && (x >= (1 << FP_MANT))) {
    return (1 << FP_MANT) - 1;
  }

  return x;
}

int32_t fp_trunc(int32_t x) {
  uint32_t res = x & 0xffff;

  if (res & 0x8000) {
    return 0xffff0000 | res;
  }

  return res;
}

int32_t to_fp(int32_t x) {
  int32_t res;
  res = fp_trunc(x << FP_EXP);

  return res;
}

int32_t fp_add(int32_t a, int32_t b) {
  return fp_trunc(a + b);
}

int32_t fp_mul(int32_t a, int32_t b) {
  return fp_trunc((a * b) >> FP_EXP);
}

int32_t fp_mul_clamp(int32_t a, int32_t b) {
  return fp_clamp((a * b) >> FP_EXP);
}

int32_t cmplx_abs_sq(cmplx_t c) {
  return fp_mul_clamp(c.real, c.real) + fp_mul_clamp(c.imag, c.imag);
}

cmplx_t cmplx_mul(cmplx_t c1, cmplx_t c2) {
  cmplx_t res;

  res.real = fp_add(fp_mul(c1.real, c2.real), -fp_mul(c1.imag, c2.imag));
  res.imag = fp_add(fp_mul(c1.real, c2.imag), fp_mul(c1.imag, c2.real));

  return res;
}

cmplx_t cmplx_sq(cmplx_t c) {
  cmplx_t res;

  res.real = fp_add(fp_mul(c.real, c.real), -fp_mul(c.imag, c.imag));
  res.imag = fp_mul(to_fp(2), fp_mul(c.real, c.imag));

  return res;
}

cmplx_t cmplx_add(cmplx_t c1, cmplx_t c2) {
  cmplx_t res;

  res.real = fp_add(c1.real, c2.real);
  res.imag = fp_add(c1.imag, c2.imag);

  return res;
}

cmplx_packed_t pack_cmplx(cmplx_t c) {
  return ((c.real & 0xffff) << 16) | (c.imag & 0xffff);
}

cmplx_t unpack_cmplx(cmplx_packed_t c) {
  cmplx_t result;

  result.real = ((int32_t)c) >> 16;
  result.imag = c & 0xffff;
  if (result.imag & 0x8000) {
    result.imag |= 0xffff0000;
  }

  return result;
}

void dump_binop_result(cmplx_t c1, cmplx_t c2, cmplx_packed_t c1_packed,
    cmplx_packed_t c2_packed, cmplx_t result, cmplx_packed_t result_packed) {
  puts("C1\n");
  puthex(c1.real);
  puts("\n");
  puthex(c1.imag);
  puts("\n");
  puts("\n");

  puts("C2\n");
  puthex(c2.real);
  puts("\n");
  puthex(c2.imag);
  puts("\n");
  puts("\n");

  puts("C1 Packed\n");
  puthex(c1_packed);
  puts("\n");
  puts("\n");

  puts("C2 Packed\n");
  puthex(c2_packed);
  puts("\n");
  puts("\n");

  puts("Result (from soft cmplx)\n");
  puthex(result.real);
  puts("\n");
  puthex(result.imag);
  puts("\n");
  puts("\n");

  puts("Result Packed (from hard cmplx insn)\n");
  puthex(result_packed);
  puts("\n");
  puts("\n");
}

int run_mul_test(cmplx_t c1, cmplx_t c2, int dump_result) {
  cmplx_packed_t c1_packed, c2_packed, result_packed;

  c1_packed = pack_cmplx(c1);
  c2_packed = pack_cmplx(c2);

  result_packed = cmplx_mul_insn(c1_packed, c2_packed);

  cmplx_t result;
  result = cmplx_mul(c1, c2);

  cmplx_t result_unpacked;
  result_unpacked = unpack_cmplx(result_packed);

  if (dump_result) {
    dump_binop_result(c1, c2, c1_packed, c2_packed, result, result_packed);
  }

  if (result_unpacked.real != result.real) {
    return 0;
  }

  if (result_unpacked.imag != result.imag) {
    return 0;
  }

  return 1;
}

int run_add_test(cmplx_t c1, cmplx_t c2, int dump_result) {
  cmplx_packed_t c1_packed, c2_packed, result_packed;

  c1_packed = pack_cmplx(c1);
  c2_packed = pack_cmplx(c2);

  result_packed = cmplx_add_insn(c1_packed, c2_packed);

  cmplx_t result;
  result = cmplx_add(c1, c2);

  cmplx_t result_unpacked;
  result_unpacked = unpack_cmplx(result_packed);

  if (dump_result) {
    dump_binop_result(c1, c2, c1_packed, c2_packed, result, result_packed);
  }

  if (result_unpacked.real != result.real) {
    return 0;
  }

  if (result_unpacked.imag != result.imag) {
    return 0;
  }

  return 1;
}

int run_abs_sq_test(cmplx_t c, int dump_results) {
  cmplx_packed_t c_packed;

  c_packed = pack_cmplx(c);

  int32_t result_soft;
  result_soft = cmplx_abs_sq(c);

  int32_t result_hard;
  result_hard = cmplx_abs_sq_insn(c_packed);

  if (dump_results) {
    puts("C\n");
    puthex(c.real);
    puts("\n");
    puthex(c.imag);
    puts("\n");
    puts("\n");

    puts("C Packed\n");
    puthex(c_packed);
    puts("\n");
    puts("\n");

    puts("Result (from soft cmplx)\n");
    puthex(result_soft);
    puts("\n");

    puts("Result (from hard cmplx)\n");
    puthex(result_hard);
    puts("\n");
  }

  if (result_hard != result_soft) {
    return 0;
  }

  return 1;
}

int main(void) {
  int failures = 0;

  for (int i = 0;i < NUM_TESTS; ++i) {
    if (!run_mul_test(test_nums[i * 2], test_nums[i * 2 + 1], 0)) {
      puts("Mul test failed: ");
      puthex(i);
      puts("\n");
      run_mul_test(test_nums[i * 2], test_nums[i * 2 + 1], 1);
      puts("\n\n");
      ++failures;
    }

    if (!run_add_test(test_nums[i * 2], test_nums[i * 2 + 1], 0)) {
      puts("Add test failed: ");
      puthex(i);
      puts("\n");
      run_add_test(test_nums[i * 2], test_nums[i * 2 + 1], 1);
      puts("\n\n");
      ++failures;
    }
  }

  for (int i = 0;i < NUM_TESTS * 2; ++i) {
    if (!run_abs_sq_test(test_nums[i], 0)) {
      puts("AbsSq test failed: ");
      puthex(i);
      puts("\n");
      run_abs_sq_test(test_nums[i], 1);
      puts("\n\n");
      ++failures;
    }
  }

  if (failures) {
    puthex(failures);
    puts(" failures seen\n");
  } else {
    puts("All tests passed!\n");
  }

  return 0;
}
