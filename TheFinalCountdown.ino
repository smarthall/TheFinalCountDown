#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <EEPROM.h>

#define DAY_SCREEN 0x72
#define HM_SCREEN 0x71
#define SERIAL_BUFFER_SIZE 32

#define SCREEN_CLEAR 0x76
#define SCREEN_DECIMAL 0x77
#define SCREEN_CURSOR 0x79

#define SCREEN_CURSOR_FIRST 0x00
#define SCREEN_CURSOR_SECOND 0x00
#define SCREEN_CURSOR_THIRD 0x00
#define SCREEN_CURSOR_FOURTH 0x00

#define SCREEN_DECIMAL_CLEAR 0x00
#define SCREEN_DECIMAL_FIRST 0x01
#define SCREEN_DECIMAL_SECOND 0x02
#define SCREEN_DECIMAL_THIRD 0x04
#define SCREEN_DECIMAL_FOURTH 0x08

#define EEPROM_COUNT_TO 0x00

const long int aminute = 60;
const long int anhour = 60 * aminute;
const long int aday = 24 * anhour;

unsigned long int countto = 0;
unsigned char serialBuffer[SERIAL_BUFFER_SIZE];
unsigned char serialBufferPos = 0;

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  setSyncProvider(RTC.get); // Setup the clock
  Serial.begin(9600); // Open a serial port
  countto = longFromEEPROM(EEPROM_COUNT_TO);
}

void loop()
{
  time_t timeNow = now();
  unsigned long int remain = countto - timeNow;
  
  unsigned int days = remain / aday;
  remain = remain % aday;
  unsigned int hours = remain / anhour;
  remain = remain % anhour;
  unsigned int minutes = remain / aminute;
  
  unsigned int tick = remain % 4;

  dayScreen(days, tick);
  hmScreen(hours, minutes);
  
  if (Serial.available() > 0) {
    unsigned char incomingByte = Serial.read();
  }
}

void dispSmallNum(unsigned char num) {
  if (num < 10) {
    Wire.write('0');
    Wire.write(num);
  } else if (num < 100) {
    Wire.write(num / 10);
    Wire.write(num % 10);
  }
}

void dayScreen(unsigned int days, unsigned char tick) {
  String strNum = String(days);
  unsigned int strLen = strNum.length();
  Wire.beginTransmission(DAY_SCREEN); // transmit to device
  Wire.write(SCREEN_CURSOR);
  Wire.write(SCREEN_CURSOR_FIRST);
  for (int i = 0; i < (4 - strLen); i++) {
    Wire.write(' ');
  }
  for (int i = 0; i < strLen; i++) {
    Wire.write(strNum[i]); // Set number
  }
  Wire.write(SCREEN_DECIMAL);
  Wire.write(0x01 << tick);
  Wire.endTransmission();    // stop transmitting
}

void hmScreen(unsigned char hours, unsigned char minutes) {
  Wire.beginTransmission(HM_SCREEN); // transmit to device
  Wire.write(SCREEN_CURSOR);
  Wire.write(SCREEN_CURSOR_FIRST);
  dispSmallNum(hours);
  dispSmallNum(minutes);
  Wire.write(SCREEN_DECIMAL);
  Wire.write(SCREEN_DECIMAL_SECOND);
  Wire.endTransmission();    // stop transmitting
}

long longFromEEPROM(unsigned int address) {
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void longToEEPROM(unsigned int address, unsigned long value) {
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

