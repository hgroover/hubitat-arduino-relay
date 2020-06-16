 /*
  Web client
 
 This sketch connects to a website (http://www.google.com)
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached
 * SainSmart 4 Relay Module connected as follows
   relay   arduino
   VCC ->  5V
   GND ->  GND
   IN1 ->  D6
   IN2 ->  D7
   IN3 ->  D8
   IN4 ->  D9
 * Power via USB from a 2A+ USB adapter. Note that 12V barrel adapter supplies
   rely on the Arduino onboard converter, which often fails on cheaper knockoffs
   like the Elegoo Uno R3. The 5V pass-thru should support the needs of the
   SainSmart relay board, which has relatively high current requirements.

 Created 13 Jun 2020 by Henry Groover
 Updated to add SSDP and respond via JSON 
 based on sketch created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe, based on work by Adrian McEwen
 */

#include "math.h"

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// Maximum length of VERSION_STR is 12
#define VERSION_STR  "1.0.16"
String version = VERSION_STR;

// Offset from UTC to local standard time in seconds (e.g. Los Angeles is -8 hours or -28800 seconds; New Delhi, 19800
// No DST - plants don't care about civic time
//const long utcOffset = -28800; // PST
const long utcOffset = -21600; // CST

// Do we have ethernet?
#define HAS_ETHERNET  1

// Do we have multicast (required for SSDP and not supported by Ethernet library)?
// May require library mentioned here: https://tkkrlab.nl/wiki/Arduino_UDP_multicast
#define HAS_MULTICAST 0

// Do we have CT connections? Set to number on analog pins 2-5
#define HAS_CURRENT_SENSOR  0

// Do we have a switch / button?
#define HAS_BUTTON 0

// 5 minutes in seconds
#define DUTY_CYCLE 300

// Startup self-test
#define WITH_TEST  0

const int buttonPin = 2;     // the number of the pushbutton pin
const int ledPin = 13;       // blinkenlights normally 13 (also used by on-board LED)
const int temperaturePinShort = A0; // Number of analog thermistor pin on short lead
const int temperaturePinLong = A1; // Long lead

int relayPin1 = 7;                 // IN1 connected to digital pin 7
int relayPin2 = 6;                 // IN2 connected to digital pin 8
int relayPin3 = 9;
int relayPin4 = 8;

int heater1On = 0; // Heater 1 on when below this temperature
int heater1Off = 15; // Heater 1 off when above this temperature
int heater2On = 0; // Heater 2 on when below this temperature
int heater2Off = 10; // Heater 2 off when above this temperature

// Requested relay states
int r1state = 0; // Relay 1 aka a
int r2state = 0; // Relay 2 aka b
int r3state = 0; // Relay 3 aka c
int r4state = 0; // Relay 4 aka d

// Last request number
int lastRequestNumber = -1; // Out of valid range 0 - 999

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
int lastTempIdx = 0;
int lastTemp2Idx = 0;

int hasTherm1 = 0;
int hasTherm2 = 0;

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
// Use the unassigned block from 00-03-00 to 00-51-ff. Note that the first 3 bytes
// will determine a "vendor" and starting with 0x00 0x3b 0xbb will yield a DHCP name WizNetxxyyzz
// where xx, yy and zz are the last three hex bytes in the MAC.
byte mac[] = {
  0x00, 0x3b, 0xBB, 0xA1, 0xDE, 0x07
};

// Change if a conflict exists. Note that port is reported over SSDP.
// 8981 is reported unused by https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
const unsigned int apiPort = 8981;      // Local port for API requests

unsigned int localPort = 8888;       // local port to listen for NTP UDP packets

// Get timeserver IP from incoming tcp ts=<ipv4>
//char timeServer[] = "pool.ntp.org"; // time.nist.gov NTP server
String timeServer = "192.168.1.1";


const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// NTP debug vars
int ntpPacketSize = 0;
unsigned long ntpLastTime = 0;

// A UDP instance to let us send and receive packets over UDP for NTP
EthernetUDP Udp;

// UDP broadcast of our status and IP address on API port
EthernetUDP UdpStatus;

#if HAS_MULTICAST
// SSDP broadcast
EthernetUDP UdpSSDP;
#endif

// Initialize the Ethernet server library
// with the IP address and port you want to use
// port 8981 needs to be set in Hubitat code as well
EthernetServer server(apiPort);

#if HAS_MULTICAST
// uuid for SSDP: 6cefb95b-72c1-4d89-974c-584dbb15b23e (UUID v4, all random numbers from uuidgenerator.net)
// SSDP info: https://stackoverflow.com/questions/13382469/ssdp-protocol-implementation
// SSDP in Chapter 1: http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
// Attempt to implement SSDP: https://forum.arduino.cc/index.php?topic=271629.0
// A library that may allow multicast (and therefore SSDP) to work: https://tkkrlab.nl/wiki/Arduino_UDP_multicast
String uuid = "6cefb95b-72c1-4d89-974c-584dbb15b23e";
#endif

int hasNet = 0;
int relayTestState = 0; // if 1, cycle through relays on startup
unsigned long ntpState = 0;

// 4.91 if input is USB power (5V) (recommended for use with relay board)
// 5.0 if using 12V input power
float vcc = 4.91;                       // only used for display purposes, if used
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
  
  // start the Ethernet connection; omit static IP to use DHCP
  if (Ethernet.begin(mac) == 0) { // Get dynamic IP
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
    // Set local port here, remote port in beginPacket
    UdpStatus.begin(apiPort);
    #if HAS_MULTICAST
    UdpSSDP.begin(1900);
    #endif
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
      sendNTPpacket(timeServer.c_str()); // send an NTP packet to a time server
      ntpState = millis();
        // wait to see if a reply is available
  delay(2000);
  ntpPacketSize = Udp.parsePacket();
  if (ntpPacketSize) {
    Serial.println("Got udp response from ntp request " + String(ntpPacketSize));
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
      hms(time_utc_sec + utcOffset);
      decTime = hms_hour * 10000L + hms_minute * 100L + hms_second;
      Serial.println("tus=" + String(time_utc_sec) + " hms " + String(hms_hour) + ":" + String(hms_minute) + " dt " + String(decTime));
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

// Get single-digit value from a token, e.g. getTokenValue("a=1;b=2;c=3", "a=", 0) returns 1
int getTokenValue(String source, String token, int defaultValue)
{
  int tokenPos = source.indexOf(token);
  if (tokenPos < 0)
  {
    return defaultValue;
  }
  return source.substring(tokenPos+token.length(), tokenPos+token.length()+1).toInt();
}

int getLengthToken(String source, String token, int numberDigits)
{
  int tokenPos = source.indexOf(token);
  if (tokenPos < 0)
  {
    return 0;
  }
  return source.substring(tokenPos + token.length(), tokenPos + token.length() + numberDigits).toInt();
}

// Read incoming UDP queries on our API port
int loop_udpquery()
{
  int queryPacket = UdpStatus.parsePacket();
  int gotQuery = 0;
  while (queryPacket)
  {
    // We've received a packet, read the data from it
    memset( packetBuffer, 0, NTP_PACKET_SIZE );
    UdpStatus.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    String s((char *)packetBuffer);
    if (s == "hgr-ardr-qry")
    {
      gotQuery = 1;
    }
    else
    {
      Serial.println("Unknown udp: " + s);
    }
    // Get next packet
    queryPacket = UdpStatus.parsePacket();
  }
  if (gotQuery)
  {
    broadcast_status();
  }
  return gotQuery;
}

int loop_tcpserver()
{
    EthernetClient client = server.available();
    if (!client) return 0;
    
    //Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String curLine;
    if (client.connected()) 
    {
      while (client.available()) 
      {
        char c = client.read();
        curLine += c;
      }
      if (curLine.startsWith("ts="))
      {
        // Not worried about DDOS as we severely time-limit NTP queries
        timeServer = curLine.substring(3);
        if (timeServer.length() >= 7)
        {
          ntpState = 0;
          Serial.println("New ts:" + timeServer);
        }
        else
        {
          Serial.println("Invalid or empty ts");
        }
        return 0;
      }
      int incomingRequest = getLengthToken(curLine, "rn=", 3);
      if (incomingRequest == lastRequestNumber)
      {
        // Drop connection and ignore
        Serial.println("Duplicate request " + String(incomingRequest));
        client.stop();
        return 0;
      }
      if (curLine.length() > 0)
      {
        lastRequestNumber = incomingRequest;
        Serial.println(curLine);
        r1state = getTokenValue(curLine, "a=", r1state);
        r2state = getTokenValue(curLine, "b=", r2state);
        r3state = getTokenValue(curLine, "c=", r3state);
        r4state = getTokenValue(curLine, "d=", r4state);
        String s;
        s = "a=" + String(r1state) + ";b=" + String(r2state) +  ";c=" + String(r3state) + ";d=" + String(r4state) + " #rcvd:" + curLine;
        client.println(s);
      }
    } // while connected
    
    // give the web browser time to receive the data
    //delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
    //Ethernet.maintain();

    // FIXME avoid sending too close to previous broadcast
    broadcast_status();
    
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
  insideTemp = Temp;
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

// Broadcast status via UDP and return bytes sent
int broadcast_status()
{
    int bytesSent =  0;
    #if 0
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0x64;
    packetBuffer[1] = 0x65;
    packetBuffer[2] = 0x66;
    packetBuffer[3] = 0x67;
    packetBuffer[4] = r1state;
    packetBuffer[5] = r2state;
    packetBuffer[6] = r3state;
    packetBuffer[7] = r4state;
    packetBuffer[8] = Ethernet.localIP()[0];
    packetBuffer[9] = Ethernet.localIP()[1];
    packetBuffer[10] = Ethernet.localIP()[2];
    packetBuffer[11] = Ethernet.localIP()[3];
    memcpy( &packetBuffer[12],  mac, 6 );
    strcpy( &packetBuffer[18], version.c_str() );
    UdpStatus.beginPacket("255.255.255.255", apiPort);
    bytesSent = UdpStatus.write(packetBuffer, 18 + 12);
    UdpStatus.endPacket();
    #else
    // Use compact JSON
    String s;
    UdpStatus.beginPacket("255.255.255.255", apiPort);
    bytesSent = UdpStatus.write("{\"hgr-ardr\":\"");
    bytesSent = bytesSent + UdpStatus.write(version.c_str());
    bytesSent = bytesSent + UdpStatus.write("\",\"ip4\":\"");
    bytesSent = bytesSent + UdpStatus.write(localIPv4().c_str());
    bytesSent = bytesSent + UdpStatus.write("\",");
    s = "\"r1\":" + String(r1state) + ",\"r2\":" + String(r2state) + ",\"r3\":" + String(r3state) + ",\"r4\":" + String(r4state) + "}";
    bytesSent = bytesSent + UdpStatus.write(s.c_str());
    UdpStatus.endPacket();
    //Serial.println("UDP bc at " + String(time_utc_sec) + " bytes " + String(bytesSent));
    #endif
    //Serial.println("UDP broadcast sent at " + String(time_utc_sec) );
    return bytesSent;
}

int loop_task()
{
  long time_delta = time_utc_sec - lastChange;
  int n;
  // Minimum delta is 1s
  if (time_delta < 0) lastChange = time_utc_sec; // Handle time wrap
  if (time_delta < 1) return 0;
  setRelay( 0, r1state );
  setRelay( 1, r2state );
  setRelay( 2, r3state );
  setRelay( 3, r4state );
  hms(time_utc_sec + utcOffset);
  decTime = hms_hour * 10000L + hms_minute * 100L + hms_second;
  //Serial.println("tus=" + String(time_utc_sec) + " hms " + String(hms_hour) + ":" + String(hms_minute) + " dt " + String(decTime));
  lastChange = time_utc_sec;
  // 10s
  if (time_utc_sec % 10 == 0 && time_utc_sec != lastFlipFlop)
  {
    // Broadcast info
    lastFlipFlop = time_utc_sec;
    if (time_utc_sec % 20 == 0)
    {
      #if HAS_MULTICAST
      // ssdp
      ssdpMulticast();
      #else
      broadcast_status();
      #endif
    }
    else
    {
      broadcast_status();
    }
  }
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
  loop_udpquery();
  loop_tcpserver();

}

// Set power relay
int setRelay( int index, int on )
{
  if (relaySetting[index] == on) return 0;
  // Optionally we could gate this with entropy check
  if (on)
  {
    digitalWrite( pins[index], LOW );
    Serial.println( "R on " + String(index)  + " p" + String(pins[index]) );
    relaySetting[index] = 1;
    // Inject delay for momentary closure
    if (on > 1)
    {
      switch (on)
      {
        case 2: delay(250); break;
        case 3: delay(300); break;
        case 4: delay(400); break;
        case 5: delay(500); break;
        case 6: delay(600); break;
        case 7: delay(750); break;
      }
      digitalWrite( pins[index], HIGH );
      relaySetting[index] = 0;
      switch (index)
      {
        case 0: r1state = 0; break;
        case 1: r2state = 0; break;
        case 2: r3state = 0; break;
        case 3: r4state = 0; break;
      }
    }
    lastRelayChange[index] = time_utc_sec;
    return 1;
  }
  else 
  {
    digitalWrite( pins[index], HIGH );
    Serial.println( "R off " + String(index) + " p" + String(pins[index]) );
    relaySetting[index] = 0;
    lastRelayChange[index] = time_utc_sec;
    return 1;
  }
  return 0; // no change
}

void printIPAddress()
{
  Serial.println("IP: " + localIPv4());
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

String localIPv4()
{
  return String(Ethernet.localIP()[0]) + "." + String(Ethernet.localIP()[1]) + "." +  String(Ethernet.localIP()[2]) + "." + String(Ethernet.localIP()[3]);
}

#if HAS_MULTICAST
// ssdp multicast via UDP to advertise
void ssdpMulticast()
{
  String s;
  if (0 == UdpSSDP.beginPacket("239.255.255.250", 1900))
  {
    Serial.println("ssdp multi beginPacket failed");
    return;
  }
  int bytesSent = 0;
  bytesSent = bytesSent +  UdpSSDP.write("NOTIFY * HTTP/1.1\r\nServer: ArdRelay/1.16\r\nHOST: 239.255.255.250:1900\r\nCACHE-CONTROL: max-age=900\r\n");
  s= "Location: http://";
  s = s + localIPv4();
  s = s + ":" + String(apiPort) + "/svc.xml\r\n";
  bytesSent = bytesSent + UdpSSDP.write(s.c_str(), s.length());
  bytesSent = bytesSent + UdpSSDP.write("NTS: ssdp:alive\r\nNT: upnp:rootdevice\r\n");
  s = "USN: uuid:" + uuid + "::upnp:rootdevice\r\n\r\n";
  bytesSent = bytesSent + UdpSSDP.write(s.c_str(), s.length());
  UdpSSDP.endPacket();
  Serial.println("Sent: " + String(bytesSent));
}
#endif

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

String SecondsPlusMS(unsigned long second_part, unsigned long ms_part )
{
  String r = String(second_part) + ".";
  if (ms_part < 10) r += "0";
  if (ms_part < 100) r += "0";
  return r + String(ms_part);
}


