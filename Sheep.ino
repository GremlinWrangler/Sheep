#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WiFi.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint64_t chipID;
char chipIDstring[10];
int16_t address16LSB=0;
//const char *ssid = "networkID";
//const char *password = "Password";


//IPAddress broadcast_ip(192, 168, 1, 255);
IPAddress broadcast_ip(192,168,0,16); //address that data is sent to
WiFiUDP Udp;

char udpBuffer[100];  //what gets sent out the UDP port
//char udpworking[100];
char udpHeader[] = {'T','E','S','T',','}; //header on UDP message

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Hello!");

   display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
    display.println(F("Booting"));
   display.display();


  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID,STAPSK);
  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
        display.println(F("WIFI failed"));
   display.display();
  } 

Udp.begin(6000);
//Udp.setBuffer(100, udpBuffer);
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  Serial.println("ID");
  chipID = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
   address16LSB = chipID; //stuff LSB of 64 bit chip ID into 16 bit

}

void loop(void)
{
  int16_t adc0, adc1, adc2, adc3;
  float volts0, volts1, volts2, volts3;

  adc0 = ads.readADC_SingleEnded(0); //ADS channel 0
  adc1 = analogRead(36);             //ESP internal ADC on pin 36
  adc2 = ads.readADC_SingleEnded(2);  //ADS channel 2
  adc3 = analogRead(39);              //ESP internal ADC on pin 39
/*
  volts0 = ads.computeVolts(adc0);
  volts1 = ads.computeVolts(adc1);
  volts2 = ads.computeVolts(adc2);
  volts3 = ads.computeVolts(adc3);
*/
  display.clearDisplay();  //writes out display with manaul 
  display.setCursor(0,0);
  display.print("ADS0="); display.print(adc0);
  display.setCursor(64,0);
  display.print("ADS2=");display.print(adc2);
  display.setCursor(0,12);
  display.print("ADC36="); display.print(adc1);
  display.setCursor(64,12);
  display.print("ADC39=");display.print(adc3);
 
  display.display();
  for (int i=0;i<5;i++){ udpBuffer[i]=udpHeader[i];} //copies header into buffer  
    for (int i=0;i<50;i++) udpBuffer[i+5]=32; //fill buffer with spaces

    loadUint16(  address16LSB,5);

    loadUint16(  adc0 ,12); //loads the four ADC values to slots in char array
    udpBuffer[11]=44;    //setting Comma
    loadUint16(  adc1 ,18);
    udpBuffer[17]=44;
    loadUint16(  adc2 ,24);
    udpBuffer[23]=44;
    loadUint16(  adc3 ,30);
    udpBuffer[29]=44;
    for (int i=0;i<40;i++){ //printing the array sent to UDP for test purposes
      display.setCursor(0+(i%15)*7,25+(i/15)*10);
      display.print(udpBuffer[i]);
    }
  Udp.beginPacket(broadcast_ip, 56700);
  Udp.print(udpBuffer);

  Udp.endPacket();
    display.display();
  delay(1000);
}

void loadUint16(int16_t inputvalue,int16_t index){ //converts a two byte 16 bit value into 4 decimal digits and loads that to array
 char workingBuffer[4];
 workingBuffer[0]=inputvalue/1000;
 int16_t remainder = inputvalue-workingBuffer[0]*1000;
 workingBuffer[1]=remainder/100;
 remainder = remainder-workingBuffer[1]*100;
  workingBuffer[2]=remainder/10;
 remainder = remainder-workingBuffer[2]*10;
 workingBuffer[3]=remainder;
 for (int i=0;i<4;i++){
  udpBuffer[index+i]=workingBuffer[i]+48; //plus 48 to convert from byte to Ascci char that is human readable.
 }
}
