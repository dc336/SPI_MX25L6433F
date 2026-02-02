// Flash opcodes
#define CMD_WREN   0x06  // Write Enable: sets WEL (Write Enable Latch). WP# must be deasserted (HIGH) to allow writes/erases.
#define CMD_RDSR   0x05  // Read Status Register: used to check WIP/WEL (busy + write enabled)
#define CMD_READ   0x03  // Read Data: opcode + 24-bit address, then clock out bytes (send 0x00 as dummy per byte)
#define CMD_PP     0x02  // Page Program: write data (1–256 bytes) to a single 256-byte page (must not cross page boundary)
#define CMD_SE_4K  0x20  // 4KB Sector Erase: erases the 4KB region containing the provided 24-bit address (bytes become 0xFF)
#define CMD_RDID   0x9F  // Read JEDEC ID: returns 3 bytes (manufacturer + device ID)
#define CMD_CE1    0x60  // Chip Erase: erases the entire chip (can take ~20–30 seconds)


#include <SPI.h>

const int FLASH_CS = 34; // GPIO34 on ESP32-S2

unsigned long readRate = 5000000; // Datasheet calls for 50MHz normal read
constexpr int chunkSize = 256;    // Due to memory limitations, larger reads require chunking


// Send a byte through SPI.h
static unsigned char spiTransfer(unsigned char b) {
  return (unsigned char)SPI.transfer(b);
}

static void csLow()  { digitalWrite(FLASH_CS, LOW);  } // Chip select line high or low
static void csHigh() { digitalWrite(FLASH_CS, HIGH); }

static void spiSendAddress24(unsigned long addr) { // Easily send 24-bit address as spiSendAddress24(0xAABBCC)
  spiTransfer((byte)(addr >> 16));
  spiTransfer((byte)(addr >> 8));
  spiTransfer((byte)(addr));
}


// Get chip identification. 3 bytes
static void readJedecId(byte id[3]) {
  csLow();
  spiTransfer(CMD_RDID);
  id[0] = spiTransfer(0x00);
  id[1] = spiTransfer(0x00);
  id[2] = spiTransfer(0x00);
  csHigh();
}


static void readData(unsigned long startAddress, unsigned char *transferBuffer, int transferLength) {
  csLow();

  spiTransfer(CMD_READ);
  spiSendAddress24(startAddress);

  // Dummy bytes required for every byte read
  for (int i = 0; i < transferLength; i++) {
    transferBuffer[i] = spiTransfer(0x00);
  }

  csHigh();
}

//Chunking to prevent memory overflow
void chunkRead(unsigned long startAddress, unsigned long transferLength) {
  unsigned char transferBuffer[chunkSize];
  unsigned long addr = startAddress;
  unsigned long remaining = transferLength;

  while (remaining > 0) {
    unsigned long n = (remaining > chunkSize) ? chunkSize : remaining;

    readData(addr, transferBuffer, (int)n);

    for (unsigned long i = 0; i < n; i++) {
      if (transferBuffer[i] < 16) Serial.print('0');
      Serial.print(transferBuffer[i], HEX);
      Serial.print((i % 16 == 15) ? "\n" : " ");
    }

    addr += n;
    remaining -= n;

    yield(); // Keep watchdog happy, this might take a while
  }
}

// Get status register
static byte readStatus() {
  csLow();
  spiTransfer(CMD_RDSR);
  byte sr = spiTransfer(0x00);
  csHigh();
  return sr;
}

static void waitWhileBusy() {
  // Write in progress == 0x01
  while (readStatus() & 0x01) {
    yield();
  }
}

// Set write enable register to allow write
static void writeEnable() {
  csLow();
  spiTransfer(CMD_WREN);
  csHigh();
}

static void chipErase() {
  writeEnable();

  csLow();
  spiTransfer(CMD_CE1);
  csHigh();

  waitWhileBusy();
}

static void sectorErase4K(unsigned long addr) {
  writeEnable();

  csLow();
  spiTransfer(CMD_SE_4K);
  spiSendAddress24(addr);
  csHigh();

  waitWhileBusy();
}

// Writes up to 256 bytes, must be in addr % 256 pages, chunking required to write beyond page or more than 256 bytes
static void pageProgram(unsigned long addr, const byte *data, int len) {
  writeEnable();

  csLow();
  spiTransfer(CMD_PP);
  spiSendAddress24(addr);
  for (int i = 0; i < len; i++) {
    spiTransfer(data[i]);
  }
  csHigh();

  waitWhileBusy();
}

static void writeByte(unsigned long addr, byte value) {
  pageProgram(addr, &value, 1);
}

void setup() {
  Serial.begin(115200);

  pinMode(FLASH_CS, OUTPUT);
  csHigh();

  // SPI.begin(SCK, MISO, MOSI, CS). Use GPIO numbers
  SPI.begin(36, 37, 35, 34);
  
  // Supports mode0 mode3 (polarities). Normal is 50MHz
  SPI.beginTransaction(SPISettings(readRate, MSBFIRST, SPI_MODE0));
  SPI.endTransaction();
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();

  int sp = line.indexOf(' ');
  String cmd  = (sp < 0) ? line : line.substring(0, sp);

  String rest = (sp < 0) ? "" : line.substring(sp + 1);
  rest.trim();

  int sp2 = rest.indexOf(' ');
  String arg1 = (sp2 < 0) ? rest : rest.substring(0, sp2);
  String arg2 = (sp2 < 0) ? ""   : rest.substring(sp2 + 1);
  arg2.trim();

  if (cmd == "read") {
    chunkRead(strtoul(arg1.c_str(), NULL, 0), strtoul(arg2.c_str(), NULL, 0)); // string to unsigned long, takes 0x or no 0x
  }

  if (cmd == "write") {
    writeByte(strtoul(arg1.c_str(), NULL, 0), strtoul(arg2.c_str(), NULL, 0));
    Serial.println("OK");
  }

  if (cmd == "erase4k") {
    sectorErase4K(strtoul(arg1.c_str(), NULL, 0));
    Serial.println("OK");
  }

  if (cmd == "id") {
    byte id[3];
    readJedecId(id);
    for (int i = 0; i < 3; i++) {
      if (id[i] < 16) Serial.print('0');
      Serial.print(id[i], HEX);
      Serial.print(i == 2 ? "\n" : " ");
    }
  }

  if (cmd == "erasechip") {
    chipErase();
    Serial.println("OK");
  }
}
