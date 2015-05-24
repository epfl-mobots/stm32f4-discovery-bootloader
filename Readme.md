# Aseba Bootloader

This bootloader allows flashing a new firmware over CAN using the aseba
protocol.

## Building

To build you have to make sure that the libopencm3 submodule is correctly
checked out. run:
```
git submodule update --init
```

Then you have to build libopencm3 using:
```
cd libopencm3
make
cd ..
```

And finally to build the bootlaoder:
```
make
```

Note: You might want to edit the aseba node id defined in config.h before building
the bootloader.

## Flashing

To flash the bootloader simply run:
```make flash```

## Using the bootloader

To flash your application using the bootloader first run asebaswitch:
```
asebaswitch "ser:device=/dev/tty.yourserialport" &
```

Then you can flash your .hex binary using:
```
asebacmd whex 42 path/to/your/binary.hex
```
(replace 42 by the aseba node id you defined in the config.h earlier)

Note: make sure that the binary you want to flash is correctly linked to use
the start address of the first flash page the bootloader uses for the application.
By default this is 0x08020000.