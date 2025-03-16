# Debug Probe for Raspberry Pi Pico

This guide explains how to set up a Raspberry Pi Pico as a debug probe for another Pico board.

## Overview

You can use one Raspberry Pi Pico board to debug another Pico board. This configuration is called "debugprobe" (formerly known as "picoprobe"). The first Pico acts as the debug adapter, connecting your computer to the target Pico that you're developing for.

## Building the Debug Probe Firmware

```bash
# Navigate to the debugprobe directory
cd Dependencies/debugprobe

# Initialize submodules if needed
git submodule update --init

# Create build directory
mkdir -p build
cd build

# Configure the build
# Note: Use PICO_BOARD=pico for Pico 1st gen and PICO_BOARD=pico2 for Pico 2nd gen
export PICO_SDK_PATH="../../pico-sdk"
cmake -DDEBUG_ON_PICO=ON -DPICO_BOARD=pico ..

# Build the firmware
make -j4
```

## Installing the Firmware

1. Connect your Pico (the one you'll use as a debug probe) to your computer while holding the BOOTSEL button
2. Release the button after connecting
3. Copy the `debugprobe_on_pico.uf2` file from the build directory to the Pico drive that appears
4. The Pico will automatically reboot with the debug probe firmware

## Making Connections

Connect your debug probe Pico (A) to the target Pico (B) as follows:

Pico A GND -> Pico B GND
Pico A GP2 -> Pico B SWCLK
Pico A GP3 -> Pico B SWDIO
Pico A GP4/UART1 TX -> Pico B GP1/UART0 RX
Pico A GP5/UART1 RX -> Pico B GP0/UART0 TX

(See "debugprobe" appendix for more details: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)

## Using the Debug Probe

Once connected, you can use the debug probe with:

- **OpenOCD**: For SWD debugging
- **GDB**: For source-level debugging
- **VS Code**: Configure launch.json to use OpenOCD for debugging

## Troubleshooting

- If the debug probe isn't detected, ensure it has the correct firmware
- Check connections between the boards
- Verify both boards have proper power
- Try a shorter or better quality USB cable

## References

- [Raspberry Pi Debug Probe Documentation](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html)
- [Raspberry Pi Pico Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)