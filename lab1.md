# Lab 1: Programming your FPGA and interacting with the demo system

Welcome to the first lab on using the [Ibex demo system](https://github.com/lowRISC/ibex-demo-system). In this lab, we will:
- Learn how to build the software.
- Program our board with a bitstream.
- Run the software on the board.
- Interact with the Ibex using GDB.

## Setting up your containter
**TODO**

## Getting started
To start please clone the repository using the following terminal commands:
```bash
git clone https://github.com/lowRISC/ibex-demo-system
cd ibex-demo-system
```

Now set up the Python virtual environment in your terminal:
```bash
# Setup python venv
python3 -m venv .
source ./bin/activate

# Install python requirements
pip3 install -r python-requirements.txt
```

You may need to repeat the PIP command if you get any errors mentioning `Failed to build wheel`.

## Building software
To build the software use the following commands in your terminal:
```bash
mkdir sw/build
cd sw/build
cmake ..
make
cd ../..
```

This builds the software that we can later use as memory content for the Ibex running in the demo system. For example, the binary for the demo application is located at `sw/build/demo/demo`.

## Building the FPGA bitstream
**TODO**

## Programming the FPGA
First let us build and install the openFPGALoader (instructions taken from [the official guide](https://trabucayre.github.io/openFPGALoader/guide/install.html)):
```bash
git clone https://github.com/trabucayre/openFPGALoader.git
cd openFPGALoader
mkdir build
cd build
cmake ..
cmake --build .
cd ../..
```

For openFPGALoader to be able to access the FPGA through the USB, we must follow the following commands:
```bash
sudo cp openFPGALoader/99-openfpgaloader.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger # force udev to take new rule
sudo usermod -a $USER -G plugdev # add user to plugdev group

```

Then we program the FPGA with the following command:
```bash
openFPGALoader/build/openFPGALoader -b arty_a7_35t <path>/<to>/lowirsc_ibex_demo_system.bit
```

## Loading the software
Before we load the software, please check that you have OpenOCD installed:
```bash
openocd --version
```
Please also check that you have version 0.11.0 or above.

If not, you can build your own OpenOCD:
```bash
git clone https://github.com/openocd-org/openocd.git
cd openocd
git checkout v0.11.0
./bootstrap
./configure
make
cd ..
export PATH=$PWD/openocd/src:$PATH
```
You must repeat the export path step if you open a new terminal window.

Then you can load and run the program as follows:
```bash
cd util
./load_demo_system.sh run ./sw/build/demo/demo
cd ..
```

Congratulations! You should now see the green LEDs zipping through and the RGB LEDs dimming up and down with different colors each time.

## Interacting with the UART
Besides the LEDs, the demo application also prints to the UART serial output. You can see this output using the following command:
```bash
screen /dev/ttyUSB1 115200
```
If you see an immediate `[screen is terminating]`, it may mean that you need super user rights. In this case, you may try adding `sudo` before the `screen` command. To exit from the `screen` command, you should press control and a together, then release these two keys and press d.

### Exercise 1
While the demo application is running try toggling the switches and buttons, you should see the input value changing.

### Exercise 2
Adjust the demo system application to print "Hello Ibex" instead of "Hello World". You should adjust the content of the file in `sw/demo/main.c`. You should rebuild the software, but do not need to rebuild the bitstream. Once you've built your updated software you should load the software onto the board to see the result.

## Debugging using GDB
We can use OpenOCD to connect to the JTAG on the FPGA. We can then use GDB to connect to OpenOCD and interact with the board as we would when we debug any other application.

First, let's load the software in the halted state:
```bash
cd util
./load_demo_system.sh halt ./sw/build/demo/demo
cd ..
```

In a separate terminal window, you can connect GDB to the OpenOCD server:
```bash
riscv32-unknown-elf-gdb ./sw/build/demo/demo
```

Inside GDB type the following command:
```
(gdb) target extended-remote localhost:3333
```

Some useful GDB commands:
- `step`: steps to the next instruction.
- `advance main`: keep running until the start of the main function (you can replace main with another function).
- `set {int}0xhex_addr = 0xhex_val`: write `hex_val` to the memory address `hex_addr`.
- `x/w 0xhex_addr`: read a word (32 bits) from `hex_addr` and display it in hexidecimal.
- `info locals`: shows you the values of the local variables for the current function.
- `backtrace`: shows you the call stack.
- `help`: to find commands you may not yet know.

### Exercise 3
Write to the green LEDs using GDB. Look in `sw/common/demo_system.h` for the value of `GPIO_BASE`. Use the set command above to write `0xa` to this base address. This should change the green LEDs to be on, off, on and off.
