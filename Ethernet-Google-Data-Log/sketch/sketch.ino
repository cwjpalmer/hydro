#include <OneWire.h>

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>

#include <SPI.h>

/* Arduino to Google Docs
 created 2011

This example code is in the public domain.

http://www.open-electronics.org


http://www.futurashop.it

https://spreadsheets.google.com/formResponse?formkey=dDBMdUx3TmQ5Y2xvX2Z0V183UVp2U0E6MQ &ifq&entry.0.single=Boris&entry.2.single=Landoni&submit=Submit
Original from http://goodsite.cocolog-nifty.com/uessay/2010/07/arduinogoogle-d.html
Modified by John Missikos 11/6/11
Modified by Andrea Fainozzi 30/6/11
Modified by Boris Landoni 8/7/11

*/

char formkey[] = "1u8whaJ5MjX6uRODEJMOgcmPT0Ar2nzNGThDDeMXshOU"; //Replace with your Key
byte mac[] = { 0x90,0xA2,0xDA,0x00,0x22,0x99};  //Replace with your Ethernet shield MAC
byte ip[] = { 192,168,0,109};  //The Arduino device IP address
byte subnet[] = { 255,255,0,0}; // replace this?!
byte gateway[] = { 192,168,0,254};
byte server[] = { 209,85,229,101 }; // Google IP

int DS18S20_Pin = 6; //DS18S20 Signal pin on digital 6

//Temperature chip i/o
OneWire ds(DS18S20_Pin);  // on digital pin 2

EthernetClient client;
 
void setup()
{
  Serial.begin(9600);
  Ethernet.begin(mac, ip , gateway , subnet);
  delay(1000);
  Serial.println("connecting...");
}
 
void loop(){
  Serial.print("Temperature = ");
  Serial.println(getTemp());
  
  String data;
  data+="";
  data+="entry.491822429"; // Temp box
  data+=20;
  data+="&submit=Submit";

  Serial.print("Data is = ");
  Serial.println(data);
 
  if (client.connect(server, 80)) {
    Serial.println("connected");
 
    client.print("POST /formResponse?formkey=");
    client.print(formkey);
    client.println("&ifq HTTP/1.1");
    client.println("Host: spreadsheets.google.com");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    client.println();
 
    Serial.print("POST /formResponse?formkey=");
    Serial.print(formkey);
    Serial.println("&ifq HTTP/1.1");
    Serial.println("Host: spreadsheets.google.com");
    Serial.println("Content-Type: application/x-www-form-urlencoded");
    Serial.println("Connection: close");
    Serial.print("Content-Length: ");
    Serial.println(data.length());
    Serial.println();
    Serial.print(data);
    Serial.println();
 
  }
  delay(1000);
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
 
  delay(10000);
 
}

int getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
  
}