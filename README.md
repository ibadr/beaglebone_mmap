# RT PREEMPT and GPIO
This little example attempts to toggle a GPIO at a preset frequency in real-time. It assumes the running Linux kernel is compiled with full RT PREEMPT support. It uses the MRAA library for controlling the GPIO pin.

# Build
gcc ./gpi.c -lmraa -o gpi

# Run
sudo ./gpi

