
/*
 * Libraries
 */
#include "DHT.h"
#include "ESP8266.h"
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

/*
 * Definitions
 */
#define DHTPIN 2    
#define DHTTYPE DHT22   
#define SSID        "MY_WIFI_SSID"
#define PASSWORD    "MY_WIFI_KEY"
#define HOST_NAME   "184.106.153.149"
#define HOST_PORT   (80)
#define THINGSPEAK_KEY    "MY_THINGSPEAK_KEY" /* Thingspeak channel key (write key) */
#define Refresh  60

/*
 * Declarations
 */
//LCD
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);
//WiFi Serial
SoftwareSerial mySerial(3, 4); /* RX:D3, TX:D2 */
//WiFi
ESP8266 wifi(mySerial);
//DHT22 sensor
DHT dht(DHTPIN, DHTTYPE);
//Buffer used to receive WiFi data
uint8_t buffer[128];
//Humidity variable
float h;
//Temperature variable
float t;
//Refresh counter
int count_down;

/*************************************************************************
 * Function : setup()
 * Process : Initial setup (executed 1 time after booting)
 * Return : -
 ***************************************************************************/
void setup() {
  //Init debug serial connection
  init_serial();
  //Init LCD screen
  init_lcd();
  //Init WiFi
  while (!init_wifi()) {
    delay(20000);
  }
  //Init DHT sensor
  init_dht();
}

/*************************************************************************
 * Function : loop()
 * Process : Loop function (executed at each cycle)
 * Return : -
 ***************************************************************************/
void loop() {
  //reset buffer
  buffer[128] = {0};

  //Data acquisition
  while (!measure()) {
    delay(5000);
    init_dht();
    delay(5000);
  }
  //Update LCD screen
  write2lcd(0, " " + String(t, 1) + "C || " + String(h, 1) + "%");

  //Update on Thingspeak
  while (!update_thingspeak()) {
    while (!init_wifi()) {
      delay(10000);
    }
    delay(10000);
  }

  count_down = Refresh;
  while(count_down >= 0){
    write2lcd(1, "Refresh in "+String(count_down)+"s");  
    count_down = count_down-1;
    delay(1000);
  }
}

/*************************************************************************
 * Function : init_serial()
 * Process : Debug serial setup.
 * Return : -
 ***************************************************************************/
void init_serial() {
  Serial.begin(9600);
  Serial.println("Serial connection setup [DONE]");
}

/*************************************************************************
 * Function : init_lcd()
 * Process : LCD setup.
 * Return : -
 ***************************************************************************/
void init_lcd() {
  Serial.println("LCD setup [START]");
  lcd.begin(16, 2);
  Serial.println("LCD setup [DONE]");
}

/*************************************************************************
 * Function : init_wifi()
 * Process : WiFi setup.
 * Return : true if success, false if failed. 
 ***************************************************************************/
bool init_wifi() {
  Serial.println("WiFi setup [START]");
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    if (wifi.disableMUX()) {
      write2lcd(1, "WiFi OK");
      Serial.println("WiFi setup [DONE]");
      return true;
    } else {
      write2lcd(1, "WiFi ERR - MUX");
      Serial.println("WiFi setup [MUX ERR]");
      return false;
    }
  } else {
    Serial.println("WiFi setup [AP ERR]");
    write2lcd(1, "WiFi ERR - AP");
    return false;
  }
}

/*************************************************************************
 * Function : init_dht()
 * Process : Sensor setup.
 * Return : -
 ***************************************************************************/
void init_dht() {
  Serial.println("DHT setup [START]");
  dht.begin();
  Serial.println("DHT setup [DONE]");
  write2lcd(1, "Sensor OK");
}

/*************************************************************************
 * Function : measure()
 * Process : Measure temperature and humidity.
 * Return : true if success, false if failed. 
 ***************************************************************************/
bool measure() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Read values [ERR]");
    write2lcd(1, "READ VAL ERR");
    return false;
  }
  else {
    return true;
  }
}

/*************************************************************************
 * Function : update_thingspeak()
 * Process : Send data on https://thingspeak.com/
 * Return : true if success, false if failed. 
 ***************************************************************************/
bool update_thingspeak() {
  uint32_t len;
  String request = "GET /update?key="+String(THINGSPEAK_KEY)+"&field1=" + String(t, 2) + "&field2=" + String(h, 2) + "\r\n\r\n";
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    Serial.println("Create tcp [OK]");
    write2lcd(1, "CREATE TCP OK");
    if (wifi.send((const uint8_t*)request.c_str(), request.length())) {
      Serial.println("Send data [OK]");
      write2lcd(1, "Send data OK");
      len = wifi.recv(buffer, sizeof(buffer), 10000);
      if (len > 0) {
        if (wifi.releaseTCP()) {
          Serial.println("Release tcp [OK]");
          return true;
        } else {
          Serial.println("Release tcp [ERR]");
          return true;
        }
      }
    }
    else {
      Serial.println("Send data [ERR]");
      write2lcd(1, "Send data ERR");
      return false;
    }
  } else {
    Serial.println("Create tcp [ERR]");
    write2lcd(1, "CREATE TCP ERR");
    return false;
  }
}

/*************************************************************************
 * Function : write2lcd()
 * Process : Write a string on the LCD string at the specified row.
 * Return : -
 ***************************************************************************/
void write2lcd(int row, String string) {
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(string);
}

