//---------------------------------------------------------------------
//ESP866 HTML Demo 02
//---------------------------------------------------------------------
// Author  : Hubert Baumgarten
//---------------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#include <TimeLib.h>
#include <time.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>               //I2C library
#include <RtcDS3231.h>    //RTC library

#include "FS.h"			//file system
#include <ArduinoJson.h>

#include "wifiPasswords.h"



#ifdef __AVR__
#include <avr/power.h>
#endif



#define BGTDEBUG 1

//LED-Stripe
#define PIN 12
//pin 4 == lua d2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ400);


//rtc and time stuff
RtcDS3231<TwoWire> rtc(Wire);

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);


//---------------------------------------------------------------------
// WiFi

byte my_WiFi_Mode = 0;  // WIFI_STA = 1 = Workstation  WIFI_AP = 2  = Accesspoint

const char * ssid_sta     = "<Your SSID>Jksdkj";
const char * password_sta = "<Your Password>sdfsdf";

const char * ssid_ap      = "ESP_HTML_02";
const char * password_ap  = "";    // alternativ :  = "12345678";

WiFiServer server(80);
WiFiClient client;

#define MAX_PACKAGE_SIZE 2048
char HTML_String[5000];
char HTTP_Header[150];

//---------------------------------------------------------------------
// Allgemeine Variablen

int Aufruf_Zaehler = 0;

#define ACTION_OK 1
#define ACTION_NOTOK 2
#define ACTION_SET_DATE_TIME 3
#define ACTION_SET_NAME 4
#define ACTION_LIES_AUSWAHL 5
#define ACTION_LIES_VOLUME 6
#define ACTION_SET_LIGHT 7

int action;

// Vor- Nachname
char Vorname[20] = "B&auml;rbel";
char Nachname[20] = "von der Waterkant";

// Uhrzeit Datum
byte Uhrzeit_HH = 16;
byte Uhrzeit_MM = 47;
byte Uhrzeit_SS = 0;
byte Datum_TT = 9;
byte Datum_MM = 2;
int Datum_JJJJ = 2016;

bool alarmIsSet = false;

int alarmColorRed, alarmColorGreen, alarmColorBlue;

// checkboxen
char Wochentage_tab[7][3] = {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};
byte Wochentage = 0;

// Radiobutton
char Jahreszeiten_tab[4][15] = {"Fr&uuml;hling", "Sommer", "Herbst", "Winter"};
byte Jahreszeit = 0;

// Combobox
char Wetter_tab[4][10] = {"Sonne", "Wolken", "Regen", "Schnee"};
byte Wetter;


// Slider
byte Volume = 15;

int colorRed = 0;
int colorGreen = 0;
int colorBlue = 0;


char tmp_string[20];
//---------------------------------------------------------------------
void setup() {
#ifdef BGTDEBUG
  Serial.begin(115200);

  for (int i = 2; i > 0; i--) {
    Serial.print("Warte ");
    Serial.print(i);
    Serial.println(" sec");
    delay(1000);
  }
#endif

  //LED init
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //---------------------------------------------------------------------
  // WiFi starten

  scanWifiNetworks();

 // WiFi_Start_STA();
 // if (my_WiFi_Mode == 0) WiFi_Start_AP();

  // get and save the current date and time

  rtc.Begin();                     //Starts I2C

  setupNTPSync();
  setRtcTimeFromNTP();
  delay(500);

  setSyncProvider(getRtcTime);

  timeClient.end();

}

//---------------------------------------------------------------------
void loop() {

	//getRtcTime();

  WiFI_Traffic();
  delay(1000);

  if (alarmIsSet == true)
  {
	  //compare current time with the alarm time
	  uint8_t alarmHour, alarmMinute;
	  getAlarmTime(&alarmHour, &alarmMinute);

	  RtcDateTime currentTime = rtc.GetDateTime();    //get the time from the RTC
	  Serial.print("current time: ");
	  Serial.print(currentTime.Hour());
	  Serial.print(":");
	  Serial.println(currentTime.Minute());

	  if (currentTime.Hour() == alarmHour & currentTime.Minute() == alarmMinute)
	  {

		  Serial.println("ALARM triggered !!");
	  }

  }

}


void scanWifiNetworks()
{
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);

	String ssidList[] = { wifiNetworks };
	String pwList[] = { wifiPassworts };
	//String ssidList[] = { "ASUS", "kartoffelsalat" };
	//String pwList[] = { "5220468835767", "dosenfutterdosenfutter" };


	Serial.println("scan start");

	// WiFi.scanNetworks will return the number of networks found
	int n = WiFi.scanNetworks();
	Serial.println("scan done");
	if (n == 0)
		Serial.println("no networks found");
	else
	{
		Serial.print(n);
		Serial.println(" networks found");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
			delay(10);
		}

		int numberOfSsids = sizeof(ssidList) / sizeof(ssidList[0]);

		//check if one ssid is in our list

		Serial.println("check list for known SSIDs");

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < numberOfSsids; j++)
			{
				if (WiFi.SSID(i).compareTo(ssidList[j]) == 0)
				{
					Serial.println("found known SSID:");
					Serial.println(ssidList[j]);

					const char* ssid = ssidList[j].c_str();
					const char* pw = pwList[j].c_str();

					setupWifi(ssid, pw);

					return;
				}
			}
		}
	}

}

void setupWifi(const char * ssid, const char* password)
{
	Serial.println("Connecting to ");
	Serial.print(ssid);

	WiFi.mode(WIFI_STA);

	bool retCode = WiFi.begin(ssid, password);

	Serial.println("Wifi begin:" + retCode);


	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("IP address assigned by DHCP: ");
	Serial.print(WiFi.localIP());

	//Serial.print(".");
	Serial.println("WiFi-Status: ");
	Serial.print(WiFi.status());

	Serial.println("Starting Server");
	server.begin();
}



//---------------------------------------------------------------------
void WiFi_Start_STA() {
  unsigned long timeout;

  WiFi.mode(WIFI_STA);   //  Workstation

  timeout = millis() + 12000L;
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    my_WiFi_Mode = WIFI_STA;

#ifdef BGTDEBUG
    Serial.print("Connected IP - Address : ");
    for (int i = 0; i < 3; i++) {
      Serial.print( WiFi.localIP()[i]);
      Serial.print(".");
    }
    Serial.println(WiFi.localIP()[3]);
#endif


  } else {
    WiFi.mode(WIFI_OFF);
#ifdef BGTDEBUG
    Serial.println("WLAN-Connection failed");
#endif

  }
}

//---------------------------------------------------------------------
void WiFi_Start_AP() {
  WiFi.mode(WIFI_AP);   // Accesspoint
  WiFi.softAP(ssid_ap, password_ap);
  server.begin();
  IPAddress myIP = WiFi.softAPIP();
  my_WiFi_Mode = WIFI_AP;

#ifdef BGTDEBUG
  Serial.print("Accesspoint started - Name : ");
  Serial.print(ssid_ap);
  Serial.print( " IP address: ");
  Serial.println(myIP);
#endif
}


void initFilerSystem()
{

	delay(1000);
	Serial.println("Mounting FS...");

	if (!SPIFFS.begin()) {
		Serial.println("Failed to mount file system");
		return;
	}
}

bool saveSettings() {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["serverName"] = "api.example.com";
	json["accessToken"] = "128du9as8du12eoue8da98h123ueh9h98";

	File configFile = SPIFFS.open("/config.json", "w");
	if (!configFile) {
		Serial.println("Failed to open config file for writing");
		return false;
	}

	json.printTo(configFile);
	return true;
}

//---------------------------------------------------------------------
void WiFI_Traffic() {

  char my_char;
  int htmlPtr = 0;
  int myIdx;
  int myIndex;
  unsigned long my_timeout;

  // Check if a client has connected
  client = server.available();
  if (!client)  {
    return;
  }

  my_timeout = millis() + 250L;
  while (!client.available() && (millis() < my_timeout) ) delay(10);
  delay(10);
  if (millis() > my_timeout)  {
    return;
  }
  //---------------------------------------------------------------------
  htmlPtr = 0;
  my_char = 0;
  while (client.available() && my_char != '\r') {
    my_char = client.read();
    HTML_String[htmlPtr++] = my_char;
  }
  client.flush();
  HTML_String[htmlPtr] = 0;
#ifdef BGTDEBUG
  exhibit ("Request : ", HTML_String);
#endif

  Aufruf_Zaehler++;



  if (Find_Start ("/?", HTML_String) < 0 && Find_Start ("GET / HTTP", HTML_String) < 0 ) {
    send_not_found();
    return;
  }

  //---------------------------------------------------------------------
  // Benutzereingaben einlesen und verarbeiten
  //---------------------------------------------------------------------
  action = Pick_Parameter_Zahl("ACTION=", HTML_String);

  // Vor und Nachname
  if ( action == ACTION_SET_NAME) {

    myIndex = Find_End("VORNAME=", HTML_String);
    if (myIndex >= 0) {
      Pick_Text(Vorname, &HTML_String[myIndex], 20);
#ifdef BGTDEBUG
      exhibit ("Vorname  : ", Vorname);
#endif
    }

    myIndex = Find_End("NACHNAME=", HTML_String);
    if (myIndex >= 0) {
      Pick_Text(Nachname, &HTML_String[myIndex], 20);
#ifdef BGTDEBUG
      exhibit ("Nachname  : ", Nachname);
#endif
    }
  }
  // Uhrzeit und Datum
  if ( action == ACTION_SET_DATE_TIME) {
    // UHRZEIT=12%3A35%3A25
    myIndex = Find_End("UHRZEIT=", HTML_String);
    if (myIndex >= 0) {
      Pick_Text(tmp_string, &HTML_String[myIndex], 8);
      Uhrzeit_HH = Pick_N_Zahl(tmp_string, ':', 1);
      Uhrzeit_MM = Pick_N_Zahl(tmp_string, ':', 2);
      Uhrzeit_SS = Pick_N_Zahl(tmp_string, ':', 3);
#ifdef BGTDEBUG
      Serial.print("Neue Uhrzeit ");
      Serial.print(Uhrzeit_HH);
      Serial.print(":");
      Serial.print(Uhrzeit_MM);
      Serial.print(":");
      Serial.println(Uhrzeit_SS);
#endif

	  alarmIsSet = true;

	  setAlarm(1, Uhrzeit_HH, Uhrzeit_MM, 0);



    }
	/*
    // DATUM=2015-12-31
    myIndex = Find_End("DATUM=", HTML_String);
    if (myIndex >= 0) {
      Pick_Text(tmp_string, &HTML_String[myIndex], 10);
      Datum_JJJJ = Pick_N_Zahl(tmp_string, '-', 1);
      Datum_MM = Pick_N_Zahl(tmp_string, '-', 2);
      Datum_TT = Pick_N_Zahl(tmp_string, '-', 3);
#ifdef BGTDEBUG
      Serial.print("Neues Datum ");
      Serial.print(Datum_TT);
      Serial.print(".");
      Serial.print(Datum_MM);
      Serial.print(".");
      Serial.println(Datum_JJJJ);
#endif
    }
	*/
  }


  if ( action == ACTION_LIES_AUSWAHL) {
    Wochentage = 0;
    for (int i = 0; i < 7; i++) {
      strcpy( tmp_string, "WOCHENTAG");
      strcati( tmp_string, i);
      strcat( tmp_string, "=");
      if (Pick_Parameter_Zahl(tmp_string, HTML_String) == 1)Wochentage |= 1 << i;
    }
    Jahreszeit = Pick_Parameter_Zahl("JAHRESZEIT=", HTML_String);
    Wetter = Pick_Parameter_Zahl("WETTER=", HTML_String);

  }

  if (action == ACTION_SET_LIGHT) {
     colorRed = Pick_Parameter_Zahl("RED=", HTML_String);
     colorGreen = Pick_Parameter_Zahl("GREEN=", HTML_String);
	 colorBlue = Pick_Parameter_Zahl("BLUE=", HTML_String);

	Serial.println("red=");
	Serial.print(colorRed);

	Serial.println("green=");
	Serial.print(colorGreen);

	Serial.println("blue=");
	Serial.print(colorBlue);

	colorWipe(strip.Color(colorRed, colorBlue, colorGreen), 50); // Red

  }


  //---------------------------------------------------------------------
  //Antwortseite aufbauen

  make_HTML01();

  //---------------------------------------------------------------------
  // Header aufbauen
  strcpy(HTTP_Header , "HTTP/1.1 200 OK\r\n");
  strcat(HTTP_Header, "Content-Length: ");
  strcati(HTTP_Header, strlen(HTML_String));
  strcat(HTTP_Header, "\r\n");
  strcat(HTTP_Header, "Content-Type: text/html\r\n");
  strcat(HTTP_Header, "Connection: close\r\n");
  strcat(HTTP_Header, "\r\n");

  /*
#ifdef BGTDEBUG
  exhibit("Header : ", HTTP_Header);
  exhibit("Laenge Header : ", strlen(HTTP_Header));
  exhibit("Laenge HTML   : ", strlen(HTML_String));
#endif
  */
  client.print(HTTP_Header);
  delay(20);

  send_HTML();

}

//---------------------------------------------------------------------
// HTML Seite 01 aufbauen
//---------------------------------------------------------------------
void make_HTML01() {

  strcpy( HTML_String, "<!DOCTYPE html>");
  strcat( HTML_String, "<html>");
  strcat( HTML_String, "<head>");
  strcat( HTML_String, "<title>HTML Demo</title>");
  strcat( HTML_String, "</head>");
  strcat( HTML_String, "<body bgcolor=\"#adcede\">");
  strcat( HTML_String, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
  strcat( HTML_String, "<h1>HTML Demo</h1>");

  /*
  //-----------------------------------------------------------------------------------------
  // Textfelder Vor- und Nachname
  strcat( HTML_String, "<h2>Textfelder</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Vorname</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"VORNAME\" maxlength=\"20\" Value =\"");
  strcat( HTML_String, Vorname);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_NAME);
  strcat( HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Nachname</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"NACHNAME\" maxlength=\"20\" Value =\"");
  strcat( HTML_String, Nachname);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br>");

  */

  //-----------------------------------------------------------------------------------------
  // Uhrzeit + Datum
  strcat( HTML_String, "<h2>Uhrzeit und Datum</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Uhrzeit</b></td>");
  strcat( HTML_String, "<td><input type=\"time\"   style= \"width:100px\" name=\"UHRZEIT\" value=\"");
  strcati2( HTML_String, Uhrzeit_HH);
  strcat( HTML_String, ":");
  strcati2( HTML_String, Uhrzeit_MM);
  strcat( HTML_String, ":");
  strcati2( HTML_String, Uhrzeit_SS);

  strcat( HTML_String, "\"></td>");

  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_DATE_TIME);
  strcat( HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat( HTML_String, "</tr>");
  /*
  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Datum</b></td>");
  strcat( HTML_String, "<td><input type=\"date\"  style= \"width:100px\" name=\"DATUM\" value=\"");
  strcati( HTML_String, Datum_JJJJ);
  strcat( HTML_String, "-");
  strcati2( HTML_String, Datum_MM);
  strcat( HTML_String, "-");
  strcati2( HTML_String, Datum_TT);
  strcat( HTML_String, "\"></td></tr>");
  */
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br>");
  /*
  //-----------------------------------------------------------------------------------------
  // Checkboxen
  strcat( HTML_String, "<h2>Checkbox, Radiobutton und Combobox</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Wochentage</b></td>");

  strcat( HTML_String, "<td>");
  for (int i = 0; i < 7; i++) {
    if (i == 5)strcat( HTML_String, "<br>");
    strcat( HTML_String, "<input type=\"checkbox\" name=\"WOCHENTAG");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" id = \"WT");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value = \"1\" ");
    if (Wochentage & 1 << i) strcat( HTML_String, "checked ");
    strcat( HTML_String, "> ");
    strcat( HTML_String, "<label for =\"WT");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, Wochentage_tab[i]);
    strcat( HTML_String, "</label>");
  }
  strcat( HTML_String, "</td>");

  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_LIES_AUSWAHL);
  strcat( HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat( HTML_String, "</tr>");

  //-----------------------------------------------------------------------------------------
  // Radiobuttons

  for (int i = 0; i < 4; i++) {
    strcat( HTML_String, "<tr>");
    if (i == 0)  strcat( HTML_String, "<td><b>Jahreszeit</b></td>");
    else strcat( HTML_String, "<td> </td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"JAHRESZEIT\" id=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if (Jahreszeit == i)strcat( HTML_String, " CHECKED");
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, Jahreszeiten_tab[i]);
    strcat( HTML_String, "</label></td></tr>");
  }

  //-----------------------------------------------------------------------------------------
  // Combobox
  strcat( HTML_String, "<tr><td><b>Wetter</b></td>");

  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<select name = \"WETTER\" style= \"width:160px\">");
  for (int i = 0; i < 4; i++) {
    strcat( HTML_String, "<option ");
    if (Wetter == i)strcat( HTML_String, "selected ");
    strcat( HTML_String, "value = \"");
    strcati( HTML_String, i);
    strcat(HTML_String, "\">");
    strcat(HTML_String, Wetter_tab[i]);
    strcat(HTML_String, "</option>");
  }
  strcat( HTML_String, "</select>");
  strcat( HTML_String, "</td></tr>");

  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br>");
  */
  //-----------------------------------------------------------------------------------------
  // Slider
  strcat( HTML_String, "<h2>Slider</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>red</b></td>");

  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"range\" name=\"RED\" min=\"0\" max=\"255\" value = \"");
  strcati(HTML_String, colorRed);
  strcat( HTML_String, "\">");
  strcat( HTML_String, "</td>");

  strcat(HTML_String, "<br>");

  strcat(HTML_String, "<td><b>green</b></td>");

  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"range\" name=\"GREEN\" min=\"0\" max=\"255\" value = \"");
  strcati(HTML_String, colorGreen);
  strcat(HTML_String, "\">");
  strcat(HTML_String, "</td>");

  strcat(HTML_String, "<br>");
  strcat(HTML_String, "<td><b>blue</b></td>");

  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"range\" name=\"BLUE\" min=\"0\" max=\"255\" value = \"");
  strcati(HTML_String, colorBlue);
  strcat(HTML_String, "\">");
  strcat(HTML_String, "</td>");

  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_LIGHT);
  strcat( HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<BR>");

  strcat( HTML_String, "<FONT SIZE=-1>");
  strcat( HTML_String, "Aufrufz&auml;hler : ");
  strcati(HTML_String, Aufruf_Zaehler);
  strcat( HTML_String, "</font>");
  strcat( HTML_String, "</font>");
  strcat( HTML_String, "</body>");
  strcat( HTML_String, "</html>");
}

//--------------------------------------------------------------------------
void send_not_found() {
#ifdef BGTDEBUG
  Serial.println("Sende Not Found");
#endif
  client.print("HTTP/1.1 404 Not Found\r\n\r\n");
  delay(20);
  client.stop();
}

//--------------------------------------------------------------------------
void send_HTML() {
  char my_char;
  int  my_len = strlen(HTML_String);
  int  my_ptr = 0;
  int  my_send = 0;

  //--------------------------------------------------------------------------
  // in Portionen senden
  while ((my_len - my_send) > 0) {
    my_send = my_ptr + MAX_PACKAGE_SIZE;
    if (my_send > my_len) {
      client.print(&HTML_String[my_ptr]);
      delay(20);
#ifdef BGTDEBUG
     // Serial.println(&HTML_String[my_ptr]);
#endif
      my_send = my_len;
    } else {
      my_char = HTML_String[my_send];
      // Auf Anfang eines Tags positionieren
      while ( my_char != '<') my_char = HTML_String[--my_send];
      HTML_String[my_send] = 0;
      client.print(&HTML_String[my_ptr]);
      delay(20);
#ifdef BGTDEBUG
     // Serial.println(&HTML_String[my_ptr]);
#endif
      HTML_String[my_send] =  my_char;
      my_ptr = my_send;
    }
  }
  client.stop();
}

//----------------------------------------------------------------------------------------------
void set_colgroup(int w1, int w2, int w3, int w4, int w5) {
  strcat( HTML_String, "<colgroup>");
  set_colgroup1(w1);
  set_colgroup1(w2);
  set_colgroup1(w3);
  set_colgroup1(w4);
  set_colgroup1(w5);
  strcat( HTML_String, "</colgroup>");

}
//------------------------------------------------------------------------------------------
void set_colgroup1(int ww) {
  if (ww == 0) return;
  strcat( HTML_String, "<col width=\"");
  strcati( HTML_String, ww);
  strcat( HTML_String, "\">");
}


//---------------------------------------------------------------------
void strcati(char* tx, int i) {
  char tmp[8];

  itoa(i, tmp, 10);
  strcat (tx, tmp);
}

//---------------------------------------------------------------------
void strcati2(char* tx, int i) {
  char tmp[8];

  itoa(i, tmp, 10);
  if (strlen(tmp) < 2) strcat (tx, "0");
  strcat (tx, tmp);
}

//---------------------------------------------------------------------
int Pick_Parameter_Zahl(const char * par, char * str) {
  int myIdx = Find_End(par, str);

  if (myIdx >= 0) return  Pick_Dec(str, myIdx);
  else return -1;
}
//---------------------------------------------------------------------
int Find_End(const char * such, const char * str) {
  int tmp = Find_Start(such, str);
  if (tmp >= 0)tmp += strlen(such);
  return tmp;
}

//---------------------------------------------------------------------
int Find_Start(const char * such, const char * str) {
  int tmp = -1;
  int ww = strlen(str) - strlen(such);
  int ll = strlen(such);

  for (int i = 0; i <= ww && tmp == -1; i++) {
    if (strncmp(such, &str[i], ll) == 0) tmp = i;
  }
  return tmp;
}
//---------------------------------------------------------------------
int Pick_Dec(const char * tx, int idx ) {
  int tmp = 0;

  for (int p = idx; p < idx + 5 && (tx[p] >= '0' && tx[p] <= '9') ; p++) {
    tmp = 10 * tmp + tx[p] - '0';
  }
  return tmp;
}
//----------------------------------------------------------------------------
int Pick_N_Zahl(const char * tx, char separator, byte n) {

  int ll = strlen(tx);
  int tmp = -1;
  byte anz = 1;
  byte i = 0;
  while (i < ll && anz < n) {
    if (tx[i] == separator)anz++;
    i++;
  }
  if (i < ll) return Pick_Dec(tx, i);
  else return -1;
}

//---------------------------------------------------------------------
int Pick_Hex(const char * tx, int idx ) {
  int tmp = 0;

  for (int p = idx; p < idx + 5 && ( (tx[p] >= '0' && tx[p] <= '9') || (tx[p] >= 'A' && tx[p] <= 'F')) ; p++) {
    if (tx[p] <= '9')tmp = 16 * tmp + tx[p] - '0';
    else tmp = 16 * tmp + tx[p] - 55;
  }

  return tmp;
}

//---------------------------------------------------------------------
void Pick_Text(char * tx_ziel, char  * tx_quelle, int max_ziel) {

  int p_ziel = 0;
  int p_quelle = 0;
  int len_quelle = strlen(tx_quelle);

  while (p_ziel < max_ziel && p_quelle < len_quelle && tx_quelle[p_quelle] && tx_quelle[p_quelle] != ' ' && tx_quelle[p_quelle] !=  '&') {
    if (tx_quelle[p_quelle] == '%') {
      tx_ziel[p_ziel] = (HexChar_to_NumChar( tx_quelle[p_quelle + 1]) << 4) + HexChar_to_NumChar(tx_quelle[p_quelle + 2]);
      p_quelle += 2;
    } else if (tx_quelle[p_quelle] == '+') {
      tx_ziel[p_ziel] = ' ';
    }
    else {
      tx_ziel[p_ziel] = tx_quelle[p_quelle];
    }
    p_ziel++;
    p_quelle++;
  }

  tx_ziel[p_ziel] = 0;
}
//---------------------------------------------------------------------
char HexChar_to_NumChar( char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 55;
  return 0;
}

#ifdef BGTDEBUG
//---------------------------------------------------------------------
void exhibit(const char * tx, int v) {
  Serial.print(tx);
  Serial.println(v);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, unsigned int v) {
  Serial.print(tx);
  Serial.println(v);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, unsigned long v) {
  Serial.print(tx);
  Serial.println(v);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, const char * v) {
  Serial.print(tx);
  Serial.println(v);
}
#endif

//LED-stuff

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
	for (uint16_t i = 0; i<strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
		strip.show();
		delay(wait);
	}
}


/*-------- NTP code ----------*/

void setupNTPSync()
{
	Serial.println("Starting NTP-UDP");
	timeClient.begin();
}


/*RTC code*/

void setRtcTimeFromNTP()
{
	Serial.println("set rtc time from ntp:");

	timeClient.update();


	Serial.println(timeClient.getEpochTime());


	time_t currentTimeEpoche = timeClient.getEpochTime();

	if (currentTimeEpoche == 0)
	{
		//could't get the time
		return;
	}
	else
	{
		RtcDateTime currentTime = RtcDateTime(year(currentTimeEpoche), month(currentTimeEpoche), day(currentTimeEpoche), hour(currentTimeEpoche), minute(currentTimeEpoche), second(currentTimeEpoche));  //define date and time object
		rtc.SetDateTime(currentTime);                                  //configure the RTC with object

		//serial time output
		char str[15];   //declare a string as an array of chars  

		sprintf(str, "%d / %d / %d %d:%d : %d",     //%d allows to print an integer to the string
			currentTime.Year(),                      //get year method
			currentTime.Month(),                 //get month method
			currentTime.Day(),                      //get day method
			currentTime.Hour(),                   //get hour method
			currentTime.Minute(),              //get minute method
			currentTime.Second()               //get second method
			);

		Serial.println(str);     //print the string to the serial port

	}


}

time_t getRtcTime()
{
	Serial.println("get the time from rtc");

	if (!rtc.IsDateTimeValid())
	{
		Serial.println("rtc time is not valid!");

	}


	RtcDateTime currentTime = rtc.GetDateTime();    //get the time from the RTC

	uint32_t currentTimeSinceEpoch = currentTime.Epoch64Time();

	Serial.print("\nSeconds since 1970 from rtc:");
	Serial.println(currentTimeSinceEpoch);

	//serial time output
	char str[15];   //declare a string as an array of chars  

	sprintf(str, "%d / %d / %d %d:%d : %d",     //%d allows to print an integer to the string
		currentTime.Year(),                      //get year method
		currentTime.Month(),                 //get month method
		currentTime.Day(),                      //get day method
		currentTime.Hour(),                   //get hour method
		currentTime.Minute(),              //get minute method
		currentTime.Second()               //get second method
		);
	Serial.println("\nformatted time: ");
	Serial.print(str);     //print the string to the serial port
}

void setAlarm(uint8_t dayOf, uint8_t hour, uint8_t minute, uint8_t second)
{
	Serial.println("set Alarm1");
	//uint8_t dayOf, uint8_t hour, uint8_t minute, uint8_t second

	DS3231AlarmOne alarm1(
		dayOf,
		hour,
		minute,
		second,
		DS3231AlarmOneControl_SecondsMatch);

	//DS3231AlarmOneControl_HoursMinutesSecondsMatch
	/*
	DS3231AlarmOne alarm1(
	0,
	0,
	0,
	10,
	DS3231AlarmOneControl_SecondsMatch);
	*/
	rtc.SetAlarmOne(alarm1);
	rtc.LatchAlarmsTriggeredFlags();

}


time_t getAlarmEpochSeconds()
{
	DS3231AlarmOne alarm1 = rtc.GetAlarmOne();

	RtcDateTime currentTime = rtc.GetDateTime();    //get the time from the RTC
	
	//for the alarm we don't use a different year or month
	time_t alarmTime = tmConvert_t((int) currentTime.Year(), currentTime.Month(), alarm1.DayOf(), alarm1.Minute(), alarm1.Minute(), alarm1.Second());
		
	return alarmTime;
}


void getAlarmTime(uint8_t *hour, uint8_t *minute)
{
	DS3231AlarmOne alarm1 = rtc.GetAlarmOne();

	Serial.print("get alarm1: ");
	Serial.print(alarm1.Hour());
	Serial.print(":");
	Serial.println(alarm1.Minute());

	*hour = alarm1.Hour();
	*minute = alarm1.Minute();
}


time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
	tmElements_t tmSet;
	tmSet.Year = YYYY - 1970;
	tmSet.Month = MM;
	tmSet.Day = DD;
	tmSet.Hour = hh;
	tmSet.Minute = mm;
	tmSet.Second = ss;
	return makeTime(tmSet);
}