# SPI_MX25L6433F
Using an ESP32-S2/S3 microcontroller to manually read, write, erase, and inspect registers on a Macronix MX25L6433F SPI NOR flash chip pulled from a WePresent WIPG-1600.


# About the chip
<img width="473" height="418" alt="image" src="https://github.com/user-attachments/assets/afaef9f2-e2de-417b-91db-42db05195202" />

The MX25L6433F is a 64 Mbit (8 MB) SPI NOR flash device supports multiple read modes, write protect, and a secured OTP area (not used by the WePresent OEM firmware).

This chip was extracted from a separate project: [WePresent WIPG-1600](https://github.com/dc336/WePresent_WIPG-1600_DOOM). 

The WIPG-1600 board exists in at least two physical variants of the same model:

- 8-pin SOP package
- 16-pin SOP package

Both variants appear to behave identically at a functional level (I’ve encountered both in random boards of the same model).

<img width="525" height="540" alt="image" src="https://github.com/user-attachments/assets/4d5c1bc7-4cef-48ec-832f-821d92c80a94" />


### Why NOR Flash is everywhere

This NOR flash acts as a bootloader (for example, U-Boot) that run before the system hands off to other storage like NAND.

NOR is commonly used for early boot because it supports execute-in-place (XIP) access, the CPU can start executing code from known mapped locations or a hardcoded entry point very early in the boot chain.

<img width="336" height="270" alt="image" src="https://github.com/user-attachments/assets/6e6514bd-b1ed-4455-9d80-27fa3683e877" />


# Chip Commands

The non-exhaustive commands used are: 

```c
#define CMD_WREN   0x06  // Write Enable: sets Write Enable Latch. WP pin must be pulled high with a 10k resistor
#define CMD_RDSR   0x05  // Read status register: used to check WIP/WEL (busy or write protection)
#define CMD_READ   0x03  // Read Data: opcode + 24-bit address, then clock out bytes (send 0x00 as dummy per byte)
#define CMD_PP     0x02  // Page Program: write data 1–256 bytes to a single 256-byte page. Can't cross page boundary, requires chunking
#define CMD_SE_4K  0x20  // 4KB Sector Erase: erases the 4KB region containing the inputted 24-bit address. Bytes erased as 0xFF
#define CMD_RDID   0x9F  // Read JEDEC ID: returns 3 bytes manufacturer + device ID
#define CMD_CE1    0x60  // Chip Erase: erases the entire chip, this can take 20–30 seconds
```

- Reads: For each byte you want to read, you clock one byte out by sending a dummy 0x00.
- Writes (PP): Flash programming can only flip bits from 1 to 0. To restore bits 0 to 1, you must erase the sector/block/chip first.

# Microcontroller

The [ESP32-S2](https://www.amazon.com/DiGiYes-ESP32-S2FN4R2-ESP32-S2-Connect-MicroPython/dp/B0BXX6R15D) is used here as a convenient SPI master

- Built in SPI with easy to use compatible Arduino library
- Fast re-build and immediate UART for debugging
- GPIO headers
- $5

<img width="1174" height="704" alt="image" src="https://github.com/user-attachments/assets/b8817f80-0424-44b4-a050-c30a85d6dc32" />


# Serial Commands


<img width="410" height="64" alt="image" src="https://github.com/user-attachments/assets/51848600-167e-4917-aa41-30b2cfe70171" />
**read <address> <length>**
Reads <length> bytes starting at <address> and prints hex
Example: read 0x000000 256

**write <address> <byte>**
Write one byte at <address>
Example: write 0x000000 0xAA

**erase4k <address>**
Erases the 4KB sector that contains <address>
Example: erase4k 0x000000

**id**
Prints 3 byte JEDEC ID
Example: id

**erasechip**
Erases the entire chip
Example: erasechip

# Interfacing

For maximum convenience, the chip was extracted from the WIPG board and soldered to a breakout board. Continuity testing can help check which pins correspond to which header pin. 

<img width="514" height="651" alt="image" src="https://github.com/user-attachments/assets/5569fa1b-d674-4eb7-88ca-35665739a54a" /><img width="443" height="279" alt="image" src="https://github.com/user-attachments/assets/744fda47-19b2-4ed3-a347-34395a633d97" />


<img width="1015" height="412" alt="image" src="https://github.com/user-attachments/assets/7dfb91d3-eee1-4f0f-9a5a-3160299d60a1" />

White wire has in-line 4.7k+4.7k reistors in series to pull WP high

# Purpose and goal? 

Labbing to get a better understanding of the ubiquitous SPI NOR Flash chip. Communicating directly to the chip and using the datasheet directly over giving the reigns to [Flashrom](https://github.com/flashrom/flashrom) gives a better understanding of how these chips work. 
