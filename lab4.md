# Lab 4: Adding Custom Instructions to Ibex

In Lab 3 we saw the difference in performance between the fixed and floating point Mandelbrot implementations, can we get further performance improvements?
We can add custom RISC-V instructions to use dedicated hardware for computing the Mandelbrot set.
This lab covers the process of adding such custom instructions, though before we look at how we add them, let's take a quick look at how you calculate a Mandelbrot set.

## Drawing the Mandelbrot set

This is not a mathematics lab, so we're not going into lots of detail here nor does it matter if you don't really understand it, it's just the 'nuts and bolts' of the calculation you need to work with.
Before we can draw the Mandelbrot set, we first need to be able to work with complex numbers.

### Complex Number Intro

A complex number can be written as $a + bi$, where $a$ and $b$ are real numbers and $i = \sqrt{-1}$ is the *imaginary unit*.
$a$ is called the *real part*, and $b$ is called the *imaginary part*.

To add complex numbers, simply sum the real and imaginary parts:

$$(a + bi) + (c + di) = (a + c) + (b + d)i$$

To mutiply complex numbers, the components are multiplied by standard algebraic rules:

$$(a + bi) (c + di) = ac + adi + bci + bdi^2 = (ac - bd) + (ad + bc)i$$

Noting that $i^2 = -1$.

Finally, the absolute value $|C|$ of a complex number $C = a + bi$ is defined as:

$$|C|^2 = a^2 + b^2$$

Noting we would need a square root to get the absolute value, but for our purposes the squared value is fine.

### Mandelbrot Set Calculation

The Mandelbrot set is a set of complex numbers.
A number $C$ is in the set if the recurrance below never diverges to infinity (i.e., $|Z_n|$ is a finite number for all $n$).

$$Z_0 = C$$

$$Z_{n+1} = Z_n^2 + C$$

It can be shown that if $|Z_n| > 2$ for any $n$, then the recurrence will diverge and hence $C$ is not part of the Mandelbrot set.

To draw the set, we map pixels to real and imaginary numbers and calculate a certain number of iterations of the $Z_n$ recurrence for each point to decide if the point is in the set:

- If $|Z_n| \leq 2$ (noting we can just test the square result against 4 to avoid a square root) for all iterations, we declare the point *in* the set.
- If $|Z_n| > 2$ for any iteration, we terminate the iterations there and declare the point *not* in the set.

We finally colour the result based upon the number of iterations we reached before making our decision.

## Complex number custom instructions

Note: These are toy instructions for demonstration use, a 'real' complex number extension may do things differently, in particular the clamping and truncation behaviour discussed below.

Our fixed point implementation of the Mandelbrot set renderer uses 16-bit numbers (12 fractional bits, 4 integer bits with 2s complement representation).
This means we can pack a complex number into a single 32-bit number.
So how about some custom instructions that implement complex number operations on the packed 32 bit representation?

We are going to want three new instructions:

* complex multiply
* complex add
* complex absolute value (squared)

These will fit into the same instruction type used by the the ALU operations (add, sub, xor etc).
All three have one destination register, multiply and add have two source registers where absolute value only has one.

### RISC-V instruction encoding

We won't be getting into full encoding details in this lab (please consult the RISC-V ISA manual volume 1 if you want more details), just the ones we need.
The bottom 7 bits of a RISC-V instruction specify the 'major opcode' (in this table, the bottom two bits are fixed to `2'b11`):

![](./lab4_imgs/RISC-V_base_opcode_map.png)

RISC-V reserves some major opcodes for custom instructions, we're going to use the *custom-0* opcode, which has a value of `7'b0001011 = 7'h0b`.
Let's call it `OPCODE_CMPLX`.

All of our instructions will be *R-type* instructions, which have the following layout:

![](./lab4_imgs/R-type_instruction_encoding.png)

The R-type instructions provide two source registers and one destination register.
We'll use the *funct3* field to select which of our operations to execute:

- `3'b000` - Complex Multiply
- `3'b001` - Complex Add
- `3'b010` - Complex Absolute Value (Squared)

*funct7* will be set to 0 in all cases.
For the absolute value operation, the *rs2* source register will always be `x0` (and ignored by the instruction).

## Adding custom instructions to Ibex

In a sense Ibex has no direct custom instruction support, however one can easily alter the RTL to add some.
There are three files in which we need to make modifications.

- `ibex_pkg.sv`: Constants and definitions
- `ibex_decoder.sv`: Instruction decoder
- `ibex_alu.sv`: Arithmetic-logical unit (ALU)

### Modifying `ibex_pkg.sv`

Open `vendor/lowrisc_ibex/rtl/ibex_pkg.sv`, take a look, and make the following changes:

1. Add our new opcode (`OPCODE_CMPLX = 7'h0b`) to the opcode enum `opcode_e`.
2. Add three new operations, one for each new instruction, to the ALU operations enum `alu_op_e`.
   This is what is produced by the decoder to tell the ALU what to do.
   Name them whatever you think is best.

### Modifying `ibex_decoder.sv`

Open `vendor/lowrisc_ibex/rtl/ibex_decoder.sv` and take a look.
The first thing to note is the decoder is split into two, one part specifies things like register read and write enables, and the other part specifies ALU related signals.
The reason for this split is timing, the decoder has two copies of the instruction in seperate flops.
With a single set of flops the 'fan-out' of those flops is very large, requiring significant buffering, slowing the logic down.
With the duplicate flops and split, the decoder fan-out is reduced, which improves performance.
Tools can do this kind of duplication automatically, but it may not be enabled in all flows (in particular in ASIC synthesis), and tools may choose a split that doesn't work as well.

Handling for `OPCODE_CMPLX` needs to be added to both decoders:

1. The first decoder begins with `unique case (opcode)`.
   For `OPCODE_CMPLX` we must set the following signals:
  - `rf_ren_a_o`/`rf_ren_b_o`: Register file read enables
  - `rf_we`: Register file write enable
  - `illegal_insn`: set 1 if the instruction is illegal (e.g. *funct3* isn't one of the 3 values we are using)
2. The second decoder controls the ALU operation and begins with `unique case (opcode_alu)`.
   We must set the following signals:
  - `alu_op_a_mux_sel_o`: Mux select for ALU operand A, always set to `OP_A_REG_A` as we always read our operands from registers for our new instructions.
  - `alu_op_b_mux_sel_o`: Mux select for ALU operand B, always set to `OP_B_REG_B` as we always read our operands from registers for our new instructions.
  - `alu_operation_o`: The ALU operation, set it to one of the new values you created in `ibex_pkg.sv` depending upon the *funct3* field of the instruction.
