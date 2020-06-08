 /*
  Web client
 
 This sketch connects to a website (http://www.google.com)
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe, based on work by Adrian McEwen
 Update 4 December 2015 hgroover - added temp / humidity sensor
 and switched to 4-position relay
 */

#include "math.h"

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
// This is the older dht11 library. The newer DHT library (supporting both dht11 and dht22)
// also requires the Adafruit Unified Sensor Library and has slightly different usage.
#include <dht11.h>

#define VERSION_STR  "1.0.12b"
// Offset from UTC to local standard time in seconds (e.g. Los Angeles is -8 hours or -28800 seconds; New Delhi, 19800
// No DST - plants don't care about civic time
//const long utcOffset = -28800; // PST
const long utcOffset = -21600; // CST

// Do we have ethernet?
#define HAS_ETHERNET  1

// Do we have CT connections? Set to number on analog pins 2-5
#define HAS_CURRENT_SENSOR  0

// Do we have a switch / button?
#define HAS_BUTTON 0

// 5 minutes in seconds
#define DUTY_CYCLE 300

// Startup self-test
#define WITH_TEST  0

dht11 DHT11;

const int buttonPin = 2;     // the number of the pushbutton pin
const int ledPin = 13;       // blinkenlights normally 13 (also used by on-board LED)
const int temperaturePinShort = A0; // Number of analog thermistor pin on short lead
const int temperaturePinLong = A1; // Long lead

int relayPin1 = 7;                 // IN1 connected to digital pin 7
int relayPin2 = 6;                 // IN2 connected to digital pin 8
int relayPin3 = 9;
int relayPin4 = 8;
int dhtPin = 3;

int heater1On = 0; // Heater 1 on when below this temperature
int heater1Off = 15; // Heater 1 off when above this temperature
int heater2On = 0; // Heater 2 on when below this temperature
int heater2Off = 10; // Heater 2 off when above this temperature

int pins[] = { 7, 6, 9, 8 };

//const int currentPin = A1; // Analog pin connected to CT circuit

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int lastButtonState = 0;
int relay1State = 0; // 0 = open

// Get request
int getRequested = 0;
int starting = 1;
unsigned long lastBlink;
int blinkState = LOW;
unsigned long lastRequest = 0;
unsigned long lastChange = 0;
unsigned long lastFlipFlop = 0;
unsigned long lastNtpCheck = 0;

// Request rate in ms
unsigned long requestRate = 30 * 1000;

// Blink rate data
#define blinkOffNormal 3500
#define blinkOnNormal 1500
#define blinkOffError 500
#define blinkOnError  2000
int blinkOff = blinkOffNormal; // ms off for normal
int blinkOn = blinkOnNormal; // ms on for normal

// Serialize requests
unsigned long requestSerial = 1;

// Measure last 3 thermistor readings to determine validity
float lastTemp[3];
float lastTemp2[3];
int lastDHT[3];
int lastHum[3];
int lastTempIdx = 0;
int lastTemp2Idx = 0;
int lastDHTIdx = 0;

int hasTherm1 = 0;
int hasTherm2 = 0;
int hasDHT = 0;

unsigned long decTime;
int hms_hour = 0;
int hms_minute = 0;
int hms_second = 0;

double outsideTemp = 0;
double waterTemp = 0;
int insideTemp = 0;
int humidity = 0;
int simulate = 0;

// Values managed by loop_time()
unsigned long time_last_millis = 0;
unsigned long time_cur_millis ;
// Set in loop_ntp() - time_utc_sec / time_utc_ms is in seconds, time_utc_millis is millis() at time of last adjustment
unsigned long time_utc_sec = 0;
unsigned long time_utc_ms = 0;
unsigned long time_utc_millis = 0;

// Light on time in decimal (000000 - 235959)
unsigned long dectime_lights_on =    70000L;
unsigned long dectime_lights_off =  180000L;

// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// This is a locally-administered address; see https://en.wikipedia.org/wiki/MAC_address
// Use the unassigned block from 00-03-00 to 00-51-ff
byte mac[] = {
  0x00, 0x3b, 0xBB, 0xA1, 0xDE, 0x07
};

unsigned int localPort = 8888;       // local port to listen for UDP packets

//char timeServer[] = "pool.ntp.org"; // time.nist.gov NTP server
char timeServer[] = "192.168.1.150"; // was 1.12, now maya  (230) or kaju-linux (150)
byte staticIP[] = {192,168,1,20};


const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// NTP debug vars
int ntpPacketSize = 0;
unsigned long ntpLastTime = 0;

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;


// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

int hasNet = 0;
int relayTestState = 0; // if 1, cycle through relays on startup
unsigned long ntpState = 0;

// 4.91 if input is USB power (5V)
// 5.0 if using recommended 12V input power
float vcc = 5.0;                       // only used for display purposes, if used
                                        // set to the measured Vcc.
float pad = 9850;                       // balance/pad resistor value, set this to
                                        // the measured resistance of your pad resistor
//float thermr = 4700; //10000;                   // thermistor nominal resistance

// Sample data at 5-minute interval for last 24 hours (60/5 * 24)
// Not enough SRAM - try 6 hours at 15 minutes
#define SAMPLE_SET  24
#define SAMPLE_INTERVAL  15
#define SAMPLE_INTERVAL_SEC  (SAMPLE_INTERVAL*60)

// With the paltry 2k of SRAM available on the Uno,
// we can't do this.
int insideHist[SAMPLE_SET] = {0};
int outsideHist[SAMPLE_SET] = {0};
int humidHist[SAMPLE_SET] = {0};
int waterHist[SAMPLE_SET] = {0};
byte settingHist[SAMPLE_SET] = {0};
int sampleHead = 0;
int sampleTail = 0;

// 0 = auto off; 1 = auto on; 2 = manual off; 3 = manual on
byte relaySetting[4] = {0,0,0,0};
// Time of last relay change as utc seconds
unsigned long lastRelayChange[4] = { 0,0,0,0 };

// manufacturer data for episco k164 10k thermistor
// simply delete this if you don't need it
// or use this idea to define your own thermistors
#define EPISCO_K164_10k 4300.0f,298.15f,10000.0f  // B,T0,R0
#define EPCOS_100k 4066.0f,298.15f,100000.0f  // B,T0,R0

#define EPCOS_4_7k  4300.0f,298.15f,4700.0f // B,T0,R0

#define CURRENT_THERMISTOR  EPCOS_4_7k
//#define CURRENT_THERMISTOR  EPCOS_4_7k

// Temperature function outputs float , the actual 
// temperature
// Temperature function inputs
// 1.AnalogInputNumber - analog input to read from 
// 2.OuputUnit - output in celsius, kelvin or fahrenheit
// 3.Thermistor B parameter - found in datasheet 
// 4.Manufacturer T0 parameter - found in datasheet (kelvin)
// 5. Manufacturer R0 parameter - found in datasheet (ohms)
// 6. Your balance resistor resistance in ohms  

float Temperature(int AnalogInputNumber,/*int OutputUnit,*/float B,float T0,float R0,float R_Balance)
{
  float R,T;

  int tempADC = analogRead(AnalogInputNumber);
  //Serial.println( "TempADC=" + String(tempADC) + " R0=" + String(int(R0)) );
  R=1024.0f*R_Balance/float(tempADC)-R_Balance;
  T=1.0f/(1.0f/T0+(1.0f/B)*log(R/R0));

//  switch(OutputUnit) {
//    case T_CELSIUS :
      T-=273.15f;
//    break;
//   case T_FAHRENHEIT :
//      T=9.0f*(T-273.15f)/5.0f+32.0f;
//    break;
//    default:
//    break;
//  };

  return T;
}

void printDouble(double val, byte precision) {
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimal places
  // example: printDouble(3.1415, 2); // prints 3.14 (two decimal places)
  Serial.print (int(val));  //prints the int part
  if( precision > 0) {
    Serial.print("."); // print the decimal point
    unsigned long frac, mult = 1;
    byte padding = precision -1;
    while(precision--) mult *=10;
    if(val >= 0) frac = (val - int(val)) * mult; else frac = (int(val) - val) * mult;
    unsigned long frac1 = frac;
    while(frac1 /= 10) padding--;
    while(padding--) Serial.print("0");
    Serial.print(frac,DEC) ;
  }
}

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  pinMode(buttonPin, INPUT); // Digital
  if (ledPin != 0) pinMode(ledPin, OUTPUT);
  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin4, OUTPUT);      // sets the digital pin as output
  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin2, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin3, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin4, HIGH);        // Prevents relays from starting up engaged

  //pinMode(temperaturePin, INPUT);
  if (ledPin != 0) digitalWrite(ledPin, blinkState);
  lastBlink = millis();
 
  Serial.println( "Relay mgr v" VERSION_STR );
  //Serial.println( "Local times are in non-civic standard time (no DST adjustment)" );
  
  int checkDHT = DHT11.read(dhtPin);
  if (checkDHT == DHTLIB_OK)
  {
    Serial.println( "Has DHT11" );
    lastDHT[lastDHTIdx] = -70;
    hasDHT = 1;
  }
  else
  {
    Serial.println( "DHT11 not detected" );
  }
  
  // start the Ethernet connection; omit static IP to use DHCP
  //if (Ethernet.begin(mac, staticIP) == 0) {
  //  Serial.println("Eth fail");
  //}
  //else
  if (Ethernet.begin(mac) == 0) { // Omit static IP to get dynamic
    Serial.println("Failed to get IP address");
    for (;;) ;
  }
  //Serial.println(Ethernet.localIP());
  if (1) {
    hasNet = 1;
    server.begin();
    // print your local IP address:
    printIPAddress();
    Udp.begin(localPort);
    //relayTestState = 1;
  }
  
}

String hms( unsigned long ts )
{
  String r;
  hms_hour = (ts  % 86400L) / 3600;
  r = String(hms_hour); // print the hour (86400 equals secs per day)
  r += ':';
  hms_minute = (ts % 3600) / 60;
  if (hms_minute < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      r += '0';
  }
  r += String(hms_minute); // print the minute (3600 equals secs per minute)
  r += ':';
  hms_second = (ts  % 60); 
  if (hms_second < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      r += '0';
   }
   r += String(hms_second); // print the second

  return r;
}

#if WITH_TEST
int loop_relaytest()
{
   if (relayTestState != 0)
   {
     switch (relayTestState)
     {
       case 1:
        delay( 5000 );
        Serial.println( "Starting relay test" );
        digitalWrite( relayPin1, LOW );
        Serial.println( "Turning on AC1" );
        delay( 2000 );
        digitalWrite( relayPin1, HIGH );
        delay( 5000 );
        relayTestState = 2;
        break;
      case 2:
        digitalWrite( relayPin2, LOW );
        Serial.println( "Turning on AC2" );
        delay( 2000 );
        digitalWrite( relayPin2, HIGH );
        delay( 5000 );
        relayTestState = 3;
        break;
      case 3:
        digitalWrite( relayPin3, LOW );
        Serial.println( "Turning on SP1" );
        delay( 2000 );
        digitalWrite( relayPin3, HIGH );
        delay( 5000 );
        relayTestState = 4;
        break;
      case 4:
        digitalWrite( relayPin4, LOW );
        Serial.println("Turning on SP2" );
        delay( 2000 );
        digitalWrite( relayPin4, HIGH );
        delay( 5000 );
        relayTestState = 0;
        break;
     }
     return 1;
   }
   return 0;
} // loop_relaytest()
#endif

int loop_ntp()
{
   if (ntpState == 0)
   {
      sendNTPpacket(timeServer); // send an NTP packet to a time server
      ntpState = millis();
        // wait to see if a reply is available
  delay(2000);
  ntpPacketSize = Udp.parsePacket();
  if (ntpPacketSize) {
    //Serial.println("Got udp response from ntp request");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Save for later display
    ntpLastTime = secsSince1900;
    //Serial.print("epoch1900 = ");
    //Serial.println(secsSince1900);

    if (secsSince1900 > 0)
    {
      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      time_utc_sec = epoch;
      time_utc_millis = millis();
      time_utc_ms = time_utc_millis % 1000;
      // print Unix time:
      Serial.println(epoch);
  
  
      // print the hour, minute and second:
      //Serial.println("The UTC time is " + hms(epoch));       // UTC is the time at Greenwich Meridian (GMT)
      /***
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if (((epoch % 3600) / 60) < 10) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ((epoch % 60) < 10) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second
      ***/
      //Serial.println("Local non-civic time is " + hms(epoch+utcOffset) );
    } // Valid NTP time received
  }
  //else Serial.println("No NTP response");

   }
   return 0;
} // loop_ntp()

int loop_dhcp()
{
  if (!hasNet) return 0;

  switch (Ethernet.maintain())
  {
    case 1:
      //renewed fail
      Serial.println("E-RNWFAIL");
      break;

    case 2:
      //renewed success
      Serial.println("RNWOK");

      //print your local IP address:
      printIPAddress();
      break;

    case 3:
      //rebind fail
      Serial.println("E-RBND");
      break;

    case 4:
      //rebind success
      Serial.println("RBNDOK");

      //print your local IP address:
      printIPAddress();
      break;

    default:
      //nothing happened
      break;

  }
  return 0;
} // loop_dhcp()


int loop_webserver()
{
    EthernetClient client = server.available();
    if (!client) return 0;
    
    //Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String curLine;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        //Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 120; url=/?r=" + String(time_utc_sec));  // refresh the page automatically every 2m
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          /***
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          ****/
          unsigned long curMillis = millis();
          client.println( "<h5>GHmgr v" VERSION_STR " " + hms(time_utc_sec + utcOffset) + " " + String(time_utc_sec + utcOffset) + " " + SecondsPlusMS(time_utc_sec, time_utc_ms) + " " + SecondsPlusMS(ntpState/1000, ntpState%1000) + " " + SecondsPlusMS(curMillis/1000, curMillis%1000) + " " + SecondsPlusMS(lastNtpCheck/1000, lastNtpCheck%1000) + "</h5>" );
          client.println( "<h6>Current</h6>" );
          client.print( "<p>In: ");
          client.print( lastDHT[lastDHTIdx] );
          client.print( " h1{" + String(heater1On) + "," + String(heater1Off) + "}" );
          client.print( " lton{" + String(dectime_lights_on) + "," + String(dectime_lights_off) + "}" );
          //client.print( " h2{" + String(heater2On)+","+String(heater2Off)+"}");
          client.print( "</p><p>Hum: ");
          client.print( lastHum[lastDHTIdx] );
          client.println( "</p>" );
          if (hasTherm1)
          {
            client.print( "<p>Out: " );
            client.print( outsideTemp );
            client.println( "</p>" );
          }
          if (hasTherm2)
          {
            client.print( "<p>Water: " );
            client.print( waterTemp );
            client.println( "</p>" );
          }
          client.println( "<p>Relays: " + String(relaySetting[0]) + "," + String(relaySetting[1]) + "," + String(relaySetting[2]) + "," + String(relaySetting[3]) + "</p>");
          client.println( "<p>NTP last: " + String(ntpLastTime) + ", pkt " + String(ntpPacketSize) + "</p>" );
          client.println( "<h6>History " + String(SAMPLE_INTERVAL) + "</h6>" );
          dumpHist( &client, "In", insideHist );
          if (hasTherm1) dumpHist( &client, "Out", outsideHist );
          if (hasTherm2) dumpHist( &client, "Water", waterHist );
          dumpHist( &client, "Hum", humidHist );
          dumpRelayHist( &client, settingHist );
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          if (curLine.startsWith("GET /"))
          {
            curLine = curLine.substring(4);
            int spaceSep = curLine.indexOf(' ');
            if (spaceSep > 0) curLine = curLine.substring(0,spaceSep);
            //Serial.println( "Got: " + curLine );
            // Handle /off=n, /on=n, /auto=n
            long arg = 0;
            int argSep = curLine.indexOf('=');
            if (argSep > 0)
            {
              arg = curLine.substring(argSep+1).toInt();
              {
                curLine = curLine.substring(1,argSep);
                //Serial.println( "Cmd: [" + curLine + "]" );
                if (curLine == "on" && arg > 0 && arg <= 4)
                {
                  digitalWrite(pins[arg-1], LOW);
                  relaySetting[arg-1] = 3;
                }
                else if (curLine == "off" && arg > 0 && arg <= 4)
                {
                  digitalWrite(pins[arg-1], HIGH);
                  relaySetting[arg-1] = 2;
                }
                else if (curLine == "auto" && arg > 0 && arg <= 4)
                {
                  digitalWrite(pins[arg-1], HIGH);
                  relaySetting[arg-1] = 0;
                }
                else if (curLine == "sim")
                {
                  simulate = arg;
                }
                else if (curLine == "h1on")
                {
                  heater1On = arg;
                }
                else if (curLine == "h1off")
                {
                  heater1Off = arg;
                }
                else if (curLine == "h2on")
                {
                  heater2On = arg;
                }
                else if (curLine == "h2off")
                {
                  heater2Off = arg;
                }
                else if (curLine == "lton")
                {
                  dectime_lights_on = arg;
                }
                else if (curLine == "ltoff")
                {
                  dectime_lights_off = arg;
                }
                //else Serial.println("n/r");
              }
            }
          }
          curLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          curLine += c;
        }
      } // client available
    } // while connected
    
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
    //Ethernet.maintain();

    return 0;
} // loop_webserver()

int loop_sample()
{
   // Check for alarm conditions
    double Temp = Temperature(A0, CURRENT_THERMISTOR, 10000.0f);
    double Temp2 = Temperature(A1, CURRENT_THERMISTOR, 10000.0f);
    #if HAS_BUTTON
    // read the state of the pushbutton value:
    buttonState = digitalRead(buttonPin);
    if (buttonState != lastButtonState)
    {
      // Door opened?
      if (buttonState == LOW && getRequested == 0) getRequested = 1;
      lastButtonState = buttonState;
    }
#endif
  if (DHT11.read(dhtPin) == DHTLIB_OK)
  {
    insideTemp = DHT11.temperature;
    if (DHT11.temperature != lastDHT[lastDHTIdx] || 
        DHT11.humidity != lastHum[lastDHTIdx])
        {
    //Serial.print("Humidity: ");
    //Serial.print(DHT11.humidity);
    //Serial.print( "% Temp(C): ");
    //Serial.println(DHT11.temperature);
          lastDHTIdx = (lastDHTIdx + 1) % 3;
          lastDHT[lastDHTIdx] = DHT11.temperature;
          lastHum[lastDHTIdx] = DHT11.humidity;
        }
  }
  if (simulate)
  {
    insideTemp = simulate;
  }
  else
  {
    insideTemp = lastDHT[lastDHTIdx];
  }
  humidity = lastHum[lastDHTIdx];
  // Temp is outside thermistor (if connected)
  if (Temp < -30 || Temp > 80)
  {
    hasTherm1 = 0;
  }
  /***
   * If thermistors are not connected, we may get random values that are in-range.
   * This logic would have them reported when the random values happen to be in-range.
  else
  {
    hasTherm1 = 1;
    outsideTemp = Temp;
  }
  ***/
  // Temp2 is water thermistor (if connected)
  if (Temp2 < -30 || Temp2 > 80)
  {
    hasTherm2 = 0;
  }
  /***
  else
  {
    hasTherm2 = 1;
    waterTemp = Temp2;
  }
  ***/
  return 0;
} // loop_sample()

int loop_led()
{
  if (insideTemp >= 32.0)
  {
    //if (blinkOn != blinkOnError)
    //{
    //  Serial.println( "Temperature high: " + String(int(Temp * 10)) );
    //}
    blinkOn = blinkOnError;
    blinkOff = blinkOffError;
  }
  else if (blinkOn != blinkOnNormal)
  {
    //Serial.println( "Temperature normal: " + String(int(Temp * 10)) );
    blinkOn = blinkOnNormal;
    blinkOff = blinkOffNormal;
  }

  unsigned long curTime = millis();
  if (curTime < lastBlink || (blinkState == LOW && curTime - lastBlink >= blinkOff) || (blinkState == HIGH && curTime - lastBlink >= blinkOn))
  {
     lastBlink = curTime;
     blinkState = (blinkState == HIGH) ? LOW : HIGH;
     //Serial.println("Time=" + String(curTime) + " state=" + String(blinkState) + " temp=" + String(int(Temp*10)) + " Temp2=" + String(int(Temp2*10)));
     if (ledPin != 0) digitalWrite(ledPin, blinkState);
  }
  return 0;
} // loop_led()

int loop_time()
{
  time_cur_millis = millis();
  long delta_since_last = time_cur_millis - time_utc_millis;
  if (time_cur_millis < time_last_millis || delta_since_last < 0)
  {
    // Force immediate resync - wrap in millis() occurs about once every 49.7 days
    ntpState = 0;
    return 0;
  }
  // Update no more than 1hz
  if (delta_since_last < 1000)
  {
    return 0;
  }
  /**
   * This should be happening in loop_task() every 2 hrs
  // Force resync every 4.8 hours
  if (delta_since_last >= 17280000)
  {
    ntpState = 0;
    return 0;
  }
   */
  time_utc_millis = time_cur_millis;
  time_utc_sec += (delta_since_last / 1000);
  time_utc_ms = time_cur_millis % 1000;
  return 0;
}

int loop_task()
{
  long time_delta = time_utc_sec - lastChange;
  int n;
  // Minimum delta is 1s
  if (time_delta < 0) lastChange = time_utc_sec; // Handle time wrap
  if (time_delta < 1) return 0;
  setHeater( 0, heater1On, heater1Off );
  //setHeater( 1, heater2On, heater2Off );
  hms(time_utc_sec + utcOffset);
  decTime = hms_hour * 10000L + hms_minute * 100L + hms_second;
  //Serial.println("tus=" + String(time_utc_sec) + " hms " + String(hms_hour) + ":" + String(hms_minute) + " dt " + String(decTime));
  // Relay 1 is light, on at 700, off at 1800
  timedValve( 1, decTime, dectime_lights_on, dectime_lights_off, 0 );
  // No valve currently
  //timedValve( 3, decTime, 73000, 73100, 9 );
  lastChange = time_utc_sec;
  // 1-minute
  // 5-minute
  /****
  if (time_utc_sec % DUTY_CYCLE== 0 && time_utc_sec != lastFlipFlop) {
    lastFlipFlop = time_utc_sec;
    for (n = 0; n < 2; n++)
    {
      if ((relaySetting[n] & 0x02) == 0) continue;
      relaySetting[n] ^= 0x01;
      if (relaySetting[n] & 0x01)
      {
        Serial.println( "t=" + String(time_utc_sec) + " r" + String(n+1) + " ON");
        digitalWrite(pins[n], LOW);
      }
      else
      {
        Serial.println( "t=" + String(time_utc_sec) + " r" + String(n+1) + " off");
        digitalWrite(pins[n], HIGH);
      }
    }
  }
  ****/
  // 10-minute
  if (time_utc_sec % SAMPLE_INTERVAL_SEC == 0)
  {
    // Save history
    //Serial.println( "Time:" + String(time_utc_sec % 600) + " " + String(sampleHead));
    insideHist[sampleHead] = insideTemp;
    outsideHist[sampleHead] = int(outsideTemp);
    humidHist[sampleHead] = humidity;
    settingHist[sampleHead] = relaySetting[0] | (relaySetting[1] << 2) | (relaySetting[2] << 4) | (relaySetting[3] << 6);
    waterHist[sampleHead] = int(waterTemp);
    sampleHead = (sampleHead + 1) % SAMPLE_SET;
    if (sampleHead == sampleTail)
    {
      sampleTail = (sampleTail + 1) % SAMPLE_SET;
    }
  }
  // 2 hour NTP adjustment - our crude method for time update produces around 12 minutes drift
  // per 24 hours. Run this check every 2 hours, or if last check was unsuccessful, every 15 minutes.
  unsigned long curM = millis();
  time_delta = (curM - lastNtpCheck) / 1000;
  if (time_delta < 0 || time_delta >= 7200 || (ntpPacketSize == 0 && time_delta >= 900))
  {
    // Last three numbers on the webserver GHmgr line are ntpState, curMillis
    // and lastNtpCheck. ntpState is the millis() at the time of the last
    // successful NTP packet receipt.
    lastNtpCheck = curM;
    if (curM < ntpState || curM - ntpState >= 7200000)
    {
      ntpState = 0;
    }
  }
  return 0;
}

void loop()
{
#if WITH_TEST
  if (loop_relaytest()) return;
#endif
  loop_ntp();
  loop_sample();
  loop_time();
  // Adjust relays @1hz, update history every 300s
  loop_task();

  loop_led();  
  loop_dhcp();
  loop_webserver();

}

// Set power relay for heater based on low (on) temp
// and high (off) temp
int setHeater( int index, int lowTemp, int highTemp )
{
  // Manual?
  if (relaySetting[index] & 0x02) return 0;
  // Enforce minimum quiet time of 10s before turning on and 30s before turning off
  if (insideTemp < lowTemp && relaySetting[index] == 0 && time_utc_sec - lastRelayChange[index] >= 10)
  {
    digitalWrite( pins[index], LOW );
    Serial.println( "H on " + String(index)  + " p" + String(pins[index]) );
    relaySetting[index] = 1;
    lastRelayChange[index] = time_utc_sec;
    return 1;
  }
  else if (insideTemp > highTemp && relaySetting[index] == 1 && time_utc_sec - lastRelayChange[index] >= 30)
  {
    digitalWrite( pins[index], HIGH );
    Serial.println( "H off " + String(index) + " p" + String(pins[index]) );
    relaySetting[index] = 0;
    lastRelayChange[index] = time_utc_sec;
    return 1;
  }
  return 0; // no change
}

void printIPAddress()
{
  Serial.print("IP: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.println();
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void dumpHist( EthernetClient *pc, char *label, int *hist )
{
  int n;
  if (sampleHead == sampleTail)
  {
    pc->println( "<p>No history " + String(label) + "</p>" );
    return;
  }
  pc->print( "<p>" ); // [" + String(sampleTail) + "," + String(sampleHead) + "] " );
  pc->print( label );
  pc->print( ":" );
  for (n = sampleTail; n != sampleHead; n = (n + 1) % SAMPLE_SET)
  {
    pc->print( " " + String(hist[n]) );
  }
  pc->println( "</p>" );
}

void dumpRelayHist( EthernetClient *pc, byte *hist )
{
  int n;
  if (sampleHead == sampleTail) return;
  pc->print( "<p>Relays: " );
  for (n = sampleTail; n != sampleHead; n = (n + 1) % SAMPLE_SET)
  {
    pc->print( " " + String(hist[n]) );
  }
  pc->println( "</p>" );
}

int timedValve( int index, long decTime, long decTimeOn, long decTimeOff, int minTemp )
{
  // Manual?
  if (relaySetting[index] & 0x02) 
  {
    Serial.println("Relay off, no valve change");
    return 0;
  }
  //Serial.print( "dectime: ");
  //Serial.print( decTime );
  //Serial.print( " on: " );
  //Serial.print( decTimeOn );
  //Serial.print( " off: " );
  //Serial.print( decTimeOff );
  if (decTime < decTimeOn || decTime >= decTimeOff)
  {
    if (relaySetting[index])
    {
      Serial.println("Assert off " + String(index) + " p" + String(pins[index]));
      relaySetting[index] = 0;
      digitalWrite( pins[index], HIGH );
      return 1;
    }
    //Serial.println("Not yet "+String(decTime));
    return 0;
   }
   if (decTime >= decTimeOn && relaySetting[index] == 0 && insideTemp >= minTemp)
   {
     Serial.println("Assert on " + String(index) + " p" + String(pins[index]));
     relaySetting[index] = 1;
     digitalWrite(pins[index], LOW);
     return 1;
   }
   // No change
   //Serial.println("Out of time " + String(decTime));
   return 0;
}

String SecondsPlusMS(unsigned long second_part, unsigned long ms_part )
{
  String r = String(second_part) + ".";
  if (ms_part < 10) r += "0";
  if (ms_part < 100) r += "0";
  return r + String(ms_part);
}


