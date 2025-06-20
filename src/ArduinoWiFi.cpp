#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h> //for communication of time and forecast to main Arduino
#include "user-secret.h"
/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015
 by Ivan Grokhotkov

 This code is in the public domain.

 */

void SerializeToJson(); // forward declaration
void sendNTPpacket(IPAddress &address); // forward declaration

const char *ssid = STASSID; // your network SSID (name)
const char *pass = STAPSK;  // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char *ntpServerName = "ntp.time.nl";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

const long utcOffsetInSeconds = 3600; //Because we are in UTC + 1:00

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

JsonDocument doc;
JsonObject WiFiDoc = doc["WiFi"].to<JsonObject>();
JsonObject TimeDoc = doc["Time"].to<JsonObject>();
JsonObject WeatherDoc = doc["Weather"].to<JsonObject>();

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp; // @suppress("Abstract class cannot be instantiated")

void setTime();


void setup() {
	Serial.begin(115200);
	Serial.println();
	Serial.println();

	// We start by connecting to a WiFi network
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	WiFiDoc["CONN"] = true;
	WiFiDoc["IP"] = WiFi.localIP();

	udp.begin(localPort);
}

void loop() {
	//get a random server from the pool
	setTime();
	SerializeToJson();
	Serial.println("");
	delay(10000);
}

void setTime(){
	WiFi.hostByName(ntpServerName, timeServerIP);
	sendNTPpacket(timeServerIP); // send an NTP packet to a time server
	// wait to see if a reply is available
	TimeDoc =
	TimeDoc["NTPQuery"] = false;
	delay(1000);

	int cb = udp.parsePacket();
	if (cb) {
		// We've received a packet, read the data from it
		udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
		//the timestamp starts at byte 40 of the received packet and is four bytes,
		// or two words, long. First, extract the two words:
		TimeDoc["NTPQuery"] = true;
		unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
		unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
		// combine the four bytes (two words) into a long integer
		// this is NTP time (seconds since Jan 1 1900):
		unsigned long secsSince1900 = highWord << 16 | lowWord;
		// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
		const unsigned long seventyYears = 2208988800UL;
		//subtract seventy years:
		//unsigned long epoch = secsSince1900 - seventyYears;
		unsigned long epoch = secsSince1900 - seventyYears + utcOffsetInSeconds;
		TimeDoc["UNIXTime"] = epoch;
		// print the hour, minute and second:
		char buffer[12];
		sprintf(buffer, "%02lu:%02lu:%02lu", (epoch % 86400L) / 3600, (epoch % 3600) / 60, epoch % 60 );
		TimeDoc["Formatted"] = buffer; // UTC is the time at Greenwich Meridian (GMT)
	}
}

void setWeather () {
	//WeatherDoc =
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(address, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}

void SerializeToJson() {
//	WeatherDoc["Station"] = "Schiphol";
//	WeatherDoc["Temperature"] = 17.1;
//	WeatherDoc["Precipitation"] = 20;
//	WeatherDoc["Pressure"] = 1013.2;
	serializeJson(doc, Serial);
}
