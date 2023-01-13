// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

module fp_mul #(
  parameter int FP_EXP = 12,
  parameter int FP_MANT = 15,
  parameter bit CLAMP = 0
) (
  input  logic [FP_MANT:0] a_i,
  input  logic [FP_MANT:0] b_i,
  output logic [FP_MANT:0] result_o
);
  localparam int MUL_RES_WIDTH = FP_MANT*2+2;

  logic [MUL_RES_WIDTH-1:0] mul_res_signed;
  logic [MUL_RES_WIDTH-1:0] mul_res_unsigned;

  assign mul_res_signed   = $signed(a_i) * $signed(b_i);
  assign mul_res_unsigned = $unsigned(mul_res_signed);

  if (CLAMP) begin
    assign result_o =
       mul_res_unsigned[MUL_RES_WIDTH-1] & ~(&mul_res_unsigned[MUL_RES_WIDTH-2:FP_MANT+FP_EXP]) ?
        {1'b1, {FP_MANT{1'b0}}}                                                           :
      ~mul_res_unsigned[MUL_RES_WIDTH-1] &  (|mul_res_unsigned[MUL_RES_WIDTH-2:FP_MANT+FP_EXP]) ?
        {1'b0, {FP_MANT{1'b1}}}                                                           :
      mul_res_unsigned[FP_MANT+FP_EXP:FP_EXP];
  end else begin
    logic unused_mul_res_unsigned;
    assign unused_mul_res_unsigned =
      ^{mul_res_unsigned[MUL_RES_WIDTH-1:FP_MANT+FP_EXP+1], mul_res_unsigned[FP_EXP-1:0]};

    assign result_o = mul_res_unsigned[FP_MANT+FP_EXP:FP_EXP];
  end
endmodule

