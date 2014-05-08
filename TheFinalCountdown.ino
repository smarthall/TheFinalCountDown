// Wire Master Writer
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>

#define DAY_SCREEN 0x72
#define HM_SCREEN 0x71

#define SCREEN_CLEAR 0x76
#define SCREEN_DECIMAL 0x77

#define SCREEN_DECIMAL_CLEAR 0x00
#define SCREEN_DECIMAL_FIRST 0x01
#define SCREEN_DECIMAL_SECOND 0x02
#define SCREEN_DECIMAL_THIRD 0x04
#define SCREEN_DECIMAL_FOURTH 0x08

const long int aminute = 60;
const long int anhour = 60 * aminute;
const long int aday = 24 * anhour;

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
  Wire.beginTransmission(DAY_SCREEN); // transmit to device
  Wire.write(SCREEN_CLEAR); // Clear Display
  Wire.write(SCREEN_DECIMAL);
  Wire.write(0x01 << tick);
  for (int i = 0; i < (4 - strNum.length()); i++) {
    Wire.write(' ');
  }
  for (int i = 0; i < strNum.length(); i++) {
    Wire.write(strNum[i]); // Set number
  }
  Wire.endTransmission();    // stop transmitting
}

void hmScreen(unsigned char hours, unsigned char minutes) {
  Wire.beginTransmission(HM_SCREEN); // transmit to device
  Wire.write(SCREEN_CLEAR); // Clear Display
  Wire.write(SCREEN_DECIMAL);
  Wire.write(SCREEN_DECIMAL_SECOND);
  dispSmallNum(hours);
  dispSmallNum(minutes);
  Wire.endTransmission();    // stop transmitting
}

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  setSyncProvider(RTC.get); // Setup the clock
  Serial.begin(9600); // Open a serial port
}

void loop()
{
  time_t timeNow = now();
  unsigned long int countto = 1399550710L + 100000;
  unsigned long int remain = countto - timeNow;
  
  unsigned int days = remain / aday;
  remain = remain % aday;
  unsigned int hours = remain / anhour;
  remain = remain % anhour;
  unsigned int minutes = remain / aminute;
  
  unsigned int tick = remain % 4;

  dayScreen(days, tick);
  hmScreen(hours, minutes);

  delay(500);
}
