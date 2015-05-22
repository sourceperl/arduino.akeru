// Loop serial Arduino console to UNB modem TD1208
#include <SoftwareSerial.h>

// Akeru UNB modem : S_RX is D4, S_TX is D5
SoftwareSerial modem(5,4); //uC Rx D5, Tx D4
// Akene UNB modem : S_RX is D5, S_TX is D4
//SoftwareSerial modem(4,5); //uC Rx D4, Tx D5

void setup()
{
  // UNB modem boot time
  delay(3000);
  // init serials
  Serial.begin(9600);
  modem.begin(9600);
}

void loop() 
{
  // loop modem to Serial
  while(modem.available()) {
    Serial.write(modem.read()); 
  }
  while(Serial.available()) {
    modem.write(Serial.read()); 
  }
}
