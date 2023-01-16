# Lab 1: Programming your FPGA and interacting with the demo system

Welcome to the first lab on using the [Ibex demo system](https://github.com/lowRISC/ibex-demo-system). In this lab, we will:

- Set up your development environment.
- Learn how to build the software.
- Program our board with a bitstream.
- Run the software on the board.
- Read from the UART.
- Interact with the Ibex using GDB.

## Getting started
To start please decompress the zip file on the USB or clone the repository using the following terminal commands:
```bash
git clone https://github.com/lowRISC/ibex-demo-system
cd ibex-demo-system
```

Now, let us set up our container. First install docker (or you can use pacman if you are already familiar with it.
On the USB go to the "Docker Installers" directory and follow the instructions based on your operating system.

### Windows
Check that you have virtualization enabled in task manager under performance and CPU. If not, you need to enable virtualization in your BIOS settings.
Make sure "Windows Subsystem for Linux" and "Virtual Machine Platform" are enabled in "Turn Windows features on or off"
Double click wsl_update_x86.msi and install.
Open powershell and run: wsl --set-default-version 2
Double click "Docker Desktop Installer Windows.exe"
When prompted select WSL 2.
If your admin account is different to your user account, you must add the user to the docker-users group. Run Computer Management as an administrator and navigate to Local Users and Groups > Groups > docker-users. Right-click to add the user to the group. Log out and log back in for the changes to take effect.

### Mac
Double click DockerMacIntel.dmg or DockerMackArm.dmg depending on your CPU and drag the Docker icon to the Applications folder.
Double click Docker.app in the Applications folder.
Accept the terms.

### Ubuntu
Make sure you have gnome-terminal installed
$ sudo apt-get install ./docker-desktop-4.16.0-ubuntu-amd64.deb
$ systemctl --user enable docker-desktop

### Debian
Make sure you have gnome-terminal installed
$ sudo apt-get install ./docker-desktop-4.16.0-debian-amd64.deb
$ systemctl --user enable docker-desktop

### Fedora
Make sure you have gnome-terminal installed
$ sudo dnf install ./docker-desktop-4.16.0-fedora-x86_64.rpm
$ systemctl --user start docker-desktop

### Set up container (Linux/Mac)
There is a prebuilt container of tools available you may want to use to get started quickly.
There are instructions for building the container for either Docker/Podman located in ./container/README.md.

A container image may be provided to you on a USB stick. You can load the containerfile by running :
```bash
sudo docker load < ibex_demo_image.tar
# OR
podman load < ibex_demo_image.tar
```

If you already have a container file, you can start the container by running :
```bash
sudo docker run -it --rm \
  -p 6080:6080 \
  -p 3333:3333 \
  -v $(pwd):/home/dev/demo:Z \
  ibex
```
OR
```bash
podman unshare chown 1000:1000 -R .
podman run -it --rm \
  -p 6080:6080 \
  -p 3333:3333 \
  -v $(pwd):/home/dev/demo:Z \
  ibex
podman unshare chown 0:0 -R .
```
To access the container once running, go to [http://localhost:6080/vnc.html](http://localhost:6080/vnc.html).

### Set up container (Windows)
Run a command prompt in administrator mode and type:
```
cd "C:\Program Files\Docker\Docker"
.\DockerCli.exe -SwitchLinuxEngine
```

Go to the folder on the USB named "Docker Images" and run:
```
docker load -i ibex_demo_image.tar
```

Go to the folder where you have decompressed the demo system repository:
```
docker run -it --rm -p 6080:6080 -p 3333:3333 -v %cd%:/home/dev/demo:Z ibex
```

## Building software
To build the software use the following commands in your terminal:
```bash
mkdir -p sw/build
pushd sw/build
cmake ..
make
popd ../..
```

This builds the software that we can later use as memory content for the Ibex running in the demo system. For example, the binary for the demo application is located at `sw/build/demo/hello_world/demo`.

## Getting the FPGA bitstream
Get the FPGA bitstream off of the USB or download the [FPGA bitstream from GitHub](https://github.com/lowRISC/ibex-demo-system/releases/download/v0.0.2/lowrisc_ibex_demo_system_0.bit). Put the bitstream at the root of your demo system repository.

Alternatively, you can build your own bitstream if you have access to Vivado by following the instructions in [the README](https://github.com/lowRISC/ibex-demo-system/blob/main/README.md).

## Programming the FPGA
Before we can program the FPGA, we need to make it accessible from inside to the container.
First, lets find out which bus and device our Arty is on:
```bash
$ lsusb
...
Bus 00X Device 00Y: ID 0403:6010 Future Technology Devices International, Ltd FT2232C/D/H Dual UART/FIFO IC
...
```
Where X and Y are numbers. Please note down what X and Y is for you (this will change if you unplug and replug your FPGA).

Then exit your docker and run it with the following parameters (assuming your running Linux):
```bash
sudo docker run -it --rm \
  -p 6080:6080 \
  -p 3333:3333 \
  -v $(pwd):/home/dev/demo:Z \
  --privileged \
  --device=/dev/bus/usb/00X/00Y \
  --device=/dev/ttyUSB1 \
  ibex
```

Then inside the container at [localhost:6080/vnc.html](http://localhost:6080/vnc.html), we program the FPGA with the following terminal command:
```bash
openFPGALoader -b arty_a7_35t \
    /home/dev/demo/lowrisc_ibex_demo_system_0.bit
```

## Loading the software
Before we load the software, please check that you have OpenOCD installed:
```bash
openocd --version
```
Please also check that you have version 0.11.0.

Then you can load and run the program as follows:
```bash
util/load_demo_system.sh run ./sw/build/demo/hello_world/demo
```

Congratulations! You should now see the green LEDs zipping through and the RGB LEDs dimming up and down with different colors each time.

## Interacting with the UART
Besides the LEDs, the demo application also prints to the UART serial output. You can see this output using the following command:
```bash
screen /dev/ttyUSB1 115200
```
If you see an immediate `[screen is terminating]`, it may mean that you need super user rights. In this case, you may try adding `sudo` before the `screen` command. To exit from the `screen` command, you should press control and a together, then release these two keys and press d.

### Exercise 1
While the demo application is running try toggling the switches and buttons, you should see changes in the input value that is displayed by `screen`.

### Exercise 2
Adjust the demo system application to print "Hello Ibex" instead of "Hello World". You should adjust the content of the file in `sw/demo/hello_world/main.c`. Afterwards, you should rebuild the software, but do not need to rebuild the bitstream. Once you've built your updated software, you can load the software onto the board to see the result.

## Debugging using GDB
We can use OpenOCD to connect to the JTAG on the FPGA. We can then use GDB to connect to OpenOCD and interact with the board as we would when we debug any other application.

First, let's load the software in the halted state:
```bash
util/load_demo_system.sh halt ./sw/build/demo/hello_world/demo
```

In a separate terminal window, you can connect GDB to the OpenOCD server:
```bash
riscv32-unknown-elf-gdb -ex "target extended-remote localhost:3333" \
    ./sw/build/demo/hello_world/demo
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
