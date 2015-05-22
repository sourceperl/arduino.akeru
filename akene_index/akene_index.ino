/*
  Sigfox index sender (network and board test)
  
  Note         :
  With card(s) : 1. Olimexino-328 (Arduino IDE set on "Duemilanove" with ATMega328)
                 https://www.olimex.com/Products/Duino/AVR/OLIMEXINO-328/
                 2. Akene V1 (Sigfox UNB modem TD1208)
                 http://snootlab.com/lang-en/snootlab-shields/889-akene-v1-en.html
  License      : MIT
  
*/

/* library */
// Arduino core
#include <SoftwareSerial.h>
// AVR low-power library (see https://github.com/rocketscream/Low-Power)
#include "LowPower.h"

// some const
#define TEST_LED     13

// some types
union payload_t 
{
  struct
  {
    uint32_t index;
  };
  uint8_t byte[12];
};

// some prototypes
void sig_send_index(uint32_t index);
uint32_t bswap_32 (uint32_t x);
uint16_t bswap_16(uint16_t x);

// some vars
// Akene UNB modem : S_RX is D5, S_TX is D4
SoftwareSerial modem(4,5); //uC Rx D4, Tx D5
uint32_t tick_8s = 0;
uint32_t index = 0;
bool debug_mode = 0;

// link stdout (printf) to Serial object
// create a FILE structure to reference our UART
static FILE console_out = {0};
// create a FILE structure to reference our UNB modem
static FILE modem_out = {0};

// create serial outputs functions
// This works because Serial.write, although of
// type virtual, already exists.
static int console_putchar (char c, FILE *stream)
{
  Serial.write(c);
  return 0;
}

static int modem_putchar (char c, FILE *stream)
{
  modem.write(c);
  return 0;
}

void setup()
{
  // IO setup
  pinMode(TEST_LED, OUTPUT);
  // test led on
  digitalWrite(TEST_LED, HIGH);
  // open serial communications, link Serial to stdio lib
  Serial.begin(9600);
  // set the data rate for the SoftwareSerial port
  modem.begin(9600);
  //modem.setTimeout(5000);
  // fill in the UART file descriptor with pointer to writer
  fdev_setup_stream(&console_out, console_putchar, NULL, _FDEV_SETUP_WRITE);
  fdev_setup_stream(&modem_out,   modem_putchar,   NULL, _FDEV_SETUP_WRITE);
  // standard output device STDOUT is console
  stdout = &console_out;
  // fill in the UART file descriptor with pointer to writer
  fdev_setup_stream(&console_out, console_putchar, NULL, _FDEV_SETUP_WRITE);
  fdev_setup_stream(&modem_out,   modem_putchar,   NULL, _FDEV_SETUP_WRITE);
  // standard output device STDOUT is console
  stdout = &console_out;
  // UNB modem boot time  
  fprintf_P(&console_out, PSTR("system startup\r\n"));
  fprintf_P(&console_out, PSTR("wait 3s for UNB modem boot time...\r\n"));
  delay(3000);
  // test led off
  digitalWrite(TEST_LED, LOW);
  // set reroute (console <-> UNB modem) ?
  fprintf_P(&console_out, PSTR("PRESS \"r\" key to reroute to UNB modem\r\n"));
  fprintf_P(&console_out, PSTR("PRESS \"d\" key to set debug mode\r\n"));
  fprintf_P(&console_out, PSTR("wait 4s...\r\n"));
  uint8_t k_loop = 0;
  char key;
  while (k_loop++ < 4)
  {
    delay(1000);
    key = Serial.read();
    if (key != -1) break;
  }
  // check user choice : debug
  if (key == 'd')
  {
    debug_mode = 1;
    fprintf_P(&console_out, PSTR("debug mode on\r\n"));
  }
  // check user choice : reroute
  else if (key == 'r') 
  {
    // debug mode : reroute console to UNB modem
    fprintf_P(&console_out, PSTR("reroute to UNB modem (reset board to exit this mode)\r\n"));
    while (1) 
    {
      // read from port 0, send to port 1:
      while (Serial.available()) 
        modem.write(Serial.read());
      // read from port 1, send to port 0:
      while (modem.available())      
        Serial.write(modem.read());
    }
  } else {
    // startup message
    fprintf_P(&console_out, PSTR("system ready\r\n"));
  }
}

void loop() {
  // vars
  char inChar = ' ';
  // update tick
  tick_8s++;
  // send Sigfox frame
  // send index: every 15 mins
  if (tick_8s % 112 == 0)
    sig_send_index(index++);
  // debug mode
  if (debug_mode) {
    fprintf_P(&console_out, PSTR("tick_8s=%d\r\n"), tick_8s);
    // arduino printf don't include "%f" for printf
    Serial.print("Index: ");
    Serial.println(index);
  }
  // ensure outgoing serial data is complete before sleep
  Serial.flush();
  modem.flush();
  // enter power down state for 8 s with ADC and BOD module disabled
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void sig_send_index(uint32_t index) {
  payload_t frame;
  char buffer[30];
  // init var
  memset(&frame, 0, sizeof(frame));
  // set frame field
  frame.index = index;
  // format hex string (ex : 0101fe5ec8990000...)
  strcpy_P(buffer, PSTR(""));
  uint8_t i = 0;
  char digits[3];
  do
  { 
    sprintf(digits, "%02x", frame.byte[i]);
    strcat(buffer, digits);
  } while(++i < sizeof(frame)); 
  // send command
  fprintf_P(&modem_out, PSTR("AT$SS=%s\r"), buffer);
  // debug message
  if (debug_mode) {
    fprintf_P(&console_out, PSTR("send \"AT$SS=%s\"\r\n"), buffer);
  }
}

// *** swap byte order function for 32 and 16 bits ***
uint32_t bswap_32(uint32_t x)
{
  unsigned char c[4], temp;

  memcpy (c, &x, 4);
  temp = c[0];
  c[0] = c[3];
  c[3] = temp;
  temp = c[1];
  c[1] = c[2];
  c[2] = temp;
  memcpy (&x, c, 4);
  return (x);
}

uint16_t bswap_16(uint16_t x)
{
  return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

