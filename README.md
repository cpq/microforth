# Microforth

Dumb Forth implementation for the bare metal microcontroller experiments.

## STM32 BluePill

In order to compile STM32 BluePill bare metal firmware with Microforth,
install ARM GCC toolchain. Ubuntu installation command:

```sh
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi gdb-arm-none-eabi openocd
```

MacOS installation command:

```sh
brew tap osx-cross/arm ; brew install arm-gcc-bin
```

Stick BluePill board into your workstation. Stick a USB-to-UART converter,
which must create a serial port. Connect RX/TX of the converter to the
BluePill's UART1 (PA9 and PA10). Enable Boot0 using a jumper as shown
on https://jeelabs.org/img/2016/DSC_5366.jpg. Reset the board. Now BluePill
is in a boot loader mode, ready to be reflashed.
Build and flash the firmware (use your `YOUR_SERIAL_PORT`):

```sh
make -C stm32-bluepill clean flash COM_PORT=YOUR_SERIAL_PORT
```

When done, disable Boot0 using a jumper. Reset the board. A PC13 built-in
LED must start blinking, and a board will run an interactive Forth
interpreter on a serial line.

Use any serial monitor program like `cu`, `minicom`, `screen`, `miniterm` to
attach to the board's serial. Use Forth word `words` to list all available
words. Example on MacOS:


```sh
miniterm.py -e /dev/cu.SLAB_USBtoUART 115200
--- Miniterm on /dev/cu.SLAB_USBtoUART  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
words
words + ok
```
