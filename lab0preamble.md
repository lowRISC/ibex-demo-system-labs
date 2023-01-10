# Open RISC-V Silicon Development with Ibex

In this tutorial, lowRISC engineers will introduce participants to working with a minimal SoC built around Ibex (the Ibex Demo System, https://github.com/lowRISC/ibex-demo-system). Ibex is a mature RISC-V (RV32IMCB) core which has seen several tape outs across academia and industry. lowRISC helps maintain Ibex as part of the OpenTitan project. The SoC is fully open source and designed to serve as a starting point for embedded computer architecture research, development, and teaching. The SoC combines Ibex with a number of useful peripherals (such as UART, Timer, PWM and SPI) and a full-featured debug interface.

The SoC has been intentionally kept simple, so people new to hardware development and/or SoC design can quickly adapt it to their needs. The labs will use the affordable (160$) Digilent Arty A7 FPGA board. A ‘simulation first’ approach is taken (using the open source Verilator simulator) to get RTL designed and working before FPGA synthesis (which is done with Xilinx Vivado). This gives a quicker, less frustrating, develop/build/debug loop.

Ibex development is funded by the OpenTitan project.
