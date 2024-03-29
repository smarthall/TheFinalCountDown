#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <EEPROM.h>

#define DAY_SCREEN 0x71
#define HM_SCREEN 0x72
#define SERIAL_BUFFER_SIZE 32

#define SCREEN_CLEAR 0x76
#define SCREEN_DECIMAL 0x77
#define SCREEN_CURSOR 0x79
#define SCREEN_BRIGHTNESS 0x7A

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
#define PROMPT ">> "

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
  setSyncInterval(600);
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
    // Read the character
    unsigned char incomingByte = Serial.read();
    Serial.write(incomingByte);
    
    // Process the input line
    if (incomingByte == '\n') {
      processSerialBuffer();
      return;
    }
    
    // Give overflow error
    if (serialBufferPos == SERIAL_BUFFER_SIZE) {
      overflowSerialBuffer();
      return;
    }
    
    // Save the byte
    serialBuffer[serialBufferPos++] = incomingByte;
  }
}

void setBrightness(unsigned char brightness) {
  Wire.beginTransmission(DAY_SCREEN);
  Wire.write(SCREEN_BRIGHTNESS);
  Wire.write(brightness);
  Wire.endTransmission();
  Wire.beginTransmission(HM_SCREEN);
  Wire.write(SCREEN_BRIGHTNESS);
  Wire.write(brightness);
  Wire.endTransmission();
}

void setTimeCommand() {
  tmElements_t toSet;
  // Check buffer is correct size
  if (serialBufferPos != 15) {
    Serial.println("E003 - Date doesn't seem to be valid");
    return;
  }
  
  // Get the input time
  toSet = readTimeFromBuffer(serialBuffer + 1);
  
  // Set the time and apply to RTC device
  setTime(makeTime(toSet));
  RTC.set(now());
  
  // Let the user know
  Serial.print("Time is: ");
  printTime(toSet);
  Serial.println("");
  
  Serial.println("Time has been set");
}

void setAlarmCommand() {
  tmElements_t toSet;
  // Check buffer is correct size
  if (serialBufferPos != 15) {
    Serial.println("E003 - Date doesn't seem to be valid");
    return;
  }
  
  // Get the input time
  toSet = readTimeFromBuffer(serialBuffer + 1);
  
  // Set the time and apply to RTC device
  countto = makeTime(toSet);
  longToEEPROM(EEPROM_COUNT_TO, countto);
  
  // Let the user know
  Serial.print("Alarm is: ");
  printTime(toSet);
  Serial.println("");
  
  Serial.println("Alarm has been set");
}

void processSerialBuffer() {
  // Blank command
  if (serialBufferPos == 0) return;
  
  // Get command Byte
  switch (serialBuffer[0]) {
    // Help
    case '?':
      showHelp();
      break;
      
    // Set Time
    case 'T':
      setTimeCommand();
      break;
      
    // Set Alarm
    case 'A':
      setAlarmCommand();
      break;
      
    // Debug
    case 'D':
      debugCommand();
      break;
      
    // Brightness
    case 'B':
      brightnessCommand();
      break;
      
    // All others
    default:
      Serial.println("E002 - Unrecognised command");
      break;
  }
  
  serialBufferPos = 0;
  Serial.print(PROMPT);
}

void brightnessCommand() {
  // Check buffer is correct size
  if (serialBufferPos != 4) {
    Serial.println("E003 - Date doesn't seem to be valid");
    return;
  }
  
  // Get the brightness, and set it
  unsigned char brightness = (unsigned char) readIntFromBuffer(serialBuffer + 1, 3);
  setBrightness(brightness);
  
  // Tell the user
  Serial.print("New Brightness: ");
  Serial.println(brightness);
}

void debugCommand() {
    tmElements_t toPrint;
    
    breakTime(now(), toPrint);
    Serial.print("Current Time: ");
    printTime(toPrint);
    Serial.println("");
    
    breakTime(countto, toPrint);
    Serial.print("Countdown Time: ");
    printTime(toPrint);
    Serial.println("");
}

long readIntFromBuffer(unsigned char *str, unsigned char len) {
  int mag = 1;
  long total = 0;
  for (int i = len - 1; i >= 0; i--) {
    unsigned char chr = str[i];
    long charval = chr - 0x30;
    total += charval * mag;
    
    mag *= 10;
  }
  
  return total;
}

tmElements_t readTimeFromBuffer(unsigned char *str) {
  tmElements_t readTime;
  
  readTime.Year = readIntFromBuffer(serialBuffer + 1, 4);
  readTime.Year = readTime.Year - 1970; // Offset from 1970
  readTime.Month = readIntFromBuffer(serialBuffer + 5, 2);
  readTime.Day = readIntFromBuffer(serialBuffer + 7, 2);
  readTime.Hour = readIntFromBuffer(serialBuffer + 9, 2);
  readTime.Minute = readIntFromBuffer(serialBuffer + 11, 2);
  readTime.Second = readIntFromBuffer(serialBuffer + 13, 2);
  
  return readTime;
}

void printTime(const tmElements_t &tm) {
  Serial.print(tm.Year + 1970);
  Serial.print("-");
  Serial.print(tm.Month);
  Serial.print("-");
  Serial.print(tm.Day);
  Serial.print(" ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.print(tm.Minute);
  Serial.print(":");
  Serial.print(tm.Second);
}

void overflowSerialBuffer() {
  Serial.println("");
  Serial.println("E001 - Command too long");
  Serial.print(PROMPT);
  serialBufferPos = 0;
}

void showHelp() {
  Serial.println("~~~ Help ~~~");
  Serial.println("Commands Available:");
  Serial.println("? - Give help");
  Serial.println("T[YYYYMMDDHHMMSS] - Set the time");
  Serial.println("A[YYYYMMDDHHMMSS] - Set the alarm");
  Serial.println("D - Debug Info");
  Serial.println("B[000] - Brightness");
  Serial.println("");
  Serial.println("");
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
  int strLen = strNum.length();
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

