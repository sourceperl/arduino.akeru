/*
  Tiny code for send periodic message to Sigfox backend
  
  This example code is in the public domain.
  Share, it's happiness !

  Note : test on plateform powered with LiPo rider Pro
  With card(s) : Akeru beta 3.2 (Arduino + Sigfox UNB modem TD1208)
                 http://snootlab.com/lang-en/shields-snootlab/547-akeru-beta-32-fr.html
*/

/* library */
// Arduino core
#include <SoftwareSerial.h>

// some const
#define TEST_LED     13
#define MSG_T_INDEX  0x03

// some types
union index_payload_t 
{
  struct
  {
    uint8_t  msg_type;
    uint8_t  id_var;
    uint32_t index_1;
    uint32_t index_2;
  };
  uint8_t byte[10];
};

// some prototypes
void sig_send_index(uint8_t id_var, uint32_t index_1, uint32_t index_2);
uint32_t bswap_32 (uint32_t x);
uint16_t bswap_16(uint16_t x);

// some vars
// Akeru UNB modem : S_RX is D4, S_TX is D5
SoftwareSerial modem(5,4); //uC Rx D5, Tx D4
uint32_t tick_1s   = 0;
bool debug_mode = true;

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
  // UNB modem boot time
  delay(3000);
  // IO setup
  pinMode(TEST_LED, OUTPUT);
  digitalWrite(TEST_LED, LOW);
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
      if (Serial.available()) 
        modem.write(Serial.read());
      // read from port 1, send to port 0:
      if (modem.available())      
        Serial.write(modem.read());
    }
  } else {
    // startup message
    fprintf_P(&console_out, PSTR("system start\r\n"));
  }
}

void loop()
{ 
  // update tick
  tick_1s++;
  // send Sigfox frame
  // send index: every 15 mins
  if (tick_1s % 900 == 0)
    sig_send_index(0x01, tick_1s, millis());
  // debug
  fprintf_P(&console_out, PSTR("tick_1s=%d\r\n"), tick_1s);
  // wait 1s
  delay(1000);
}

void sig_send_index(uint8_t id_var, uint32_t index_1, uint32_t index_2)
{
  index_payload_t frame;
  char buffer[30];
  // init var
  memset(&frame, 0, sizeof(frame));
  // set frame field
  frame.msg_type = MSG_T_INDEX;
  frame.id_var   = id_var;
  frame.index_1  = bswap_32(index_1);
  frame.index_2  = bswap_32(index_2);
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

