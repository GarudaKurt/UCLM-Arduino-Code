#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>

#ifdef ESP32
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <WiFiUdp.h>
#endif

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include "Buttons.h"
#include "Distance.h"

Buttons btn;
Distance get;

Adafruit_MPU6050 mpu;


#define WIFI_SSID "teloy"
#define WIFI_PASSWORD "Tyler_property#0124"
#define API_KEY "AIzaSyCf_yEMzkxTZTZvhxnIUfZl3rlqdWRE9Yo" //firebase realtime database API_KEY
#define DATABASE_URL "iot-healthcare-72d91-default-rtdb.asia-southeast1.firebasedatabase.app/" //OAuth realtime database
#define USER_EMAIL "test@gmail.com" //create OAuth user email
#define USER_PASSWORD "test123456" //create OAuth user password

int timeZone = 8 * 3600;  // Philippine Time
int dst = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long prevMillis = 0;
const long interval = 1000;

String falling = "";


#define sda1 D2
#define scl1 D1
#define sda2 D4
#define scl2 D5

#define mpuI2C 0x3C
#define lcdI2C 0x68

LiquidCrystal_I2C lcd(0x27, 16, 2);  


void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to wifi please check!");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Wire.begin(sda1, scl1);
  Serial.println("Intialized mpu I2C...");
  Wire.beginTransmission(mpuI2C);
  Wire.endTransmission();

  Wire.begin(sda2, scl2);
  Serial.println("Intialized LCD I2C...");
  Wire.beginTransmission(lcdI2C);
  Wire.endTransmission();

  lcd.clear();
  lcd.init();
  lcd.backlight();
  
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  configTime(timeZone, dst, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time....");
  while(!time(nullptr)){
    Serial.println("Time is nullptr!");
    delay(1000);
  }
  Serial.println("\nTime response... OK!");

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(4096, 1024); // Set SSL buffer size
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);

  Serial.println("Setup complete.");
  Serial.println("");
  delay(100);
}

void scrollText(String message) {
  static int scrollPosition = 0;

  // Clear the display
  lcd.clear();
  
  lcd.setCursor(0, 0);
  
  lcd.print(message.substring(scrollPosition, scrollPosition + 16));
  
  lcd.setCursor(0, 1);
  
  lcd.print(message.substring(0, scrollPosition));
  
  scrollPosition++;
  
  if (scrollPosition >= message.length()) {
    scrollPosition = 0;
  }
}



void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // /* Print out the values */
  // Serial.print("Acceleration X: ");
  // Serial.print(a.acceleration.x);
  // Serial.print(", Y: ");
  // Serial.print(a.acceleration.y);
  // Serial.print(", Z: ");
  // Serial.print(a.acceleration.z);
  // Serial.println(" m/s^2");

    // Calculate acceleration magnitude
  float accelMagnitude = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
  bool fallState = false;
  // Check if a fall is detected (you may need to adjust threshold based on testing)
  if (accelMagnitude < 120.0 && get.totalDistance() < 20 ) {
    Serial.println("Heavy Fall detected!");
    falling = "heavy fall";
    fallState = true;
    Serial.print("[DEBUG]Heavy accelMagnitude");
    Serial.println(accelMagnitude);
  } else if(accelMagnitude > 130.0 && get.totalDistance() > 20 && get.totalDistance() < 40) {
    Serial.println("Light Fall detected!");
    falling = "light fall";
    Serial.print("[DEBUG]Light accelMagnitude");
    Serial.println(accelMagnitude);
    fallState = true;
  }

  scrollText(falling);

  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.println(temp.temperature);

  Serial.print("[DEBUG] Ultrasonic ");
  Serial.println(get.totalDistance());

  char buffer[10];
  dtostrf(temp.temperature, 8, 5, buffer);
  String val =  String(buffer);
 

  Serial.print("Temperature:");
  Serial.println(temp.temperature);
  
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  String Time = String(p_tm->tm_hour) + ":" + String(p_tm->tm_min) + ":" + String(p_tm->tm_sec) +" "+String(p_tm->tm_mday) + "/" + String(p_tm->tm_mon + 1) ;

  Serial.print("[DEBUG] Date time");
  Serial.println(Time);

  if (Firebase.ready()) { 

      if(fallState) {
        bool badgeTime = Firebase.setString(fbdo, F("/dataInfo/timeFall"), Time.c_str());
        if(!badgeTime) {
          Serial.print("Failed to send time ");
          Serial.println(fbdo.errorReason());
        }
        bool isPersonfall = Firebase.setString(fbdo, F("/dataInfo/falling"), falling.c_str());
        if(!isPersonfall) {
          Serial.print("Failed to send fall data: ");
          Serial.println(fbdo.errorReason());
        }
        fallState = false;
      }

      Serial.print("Value of val");
      Serial.println(val.c_str());
      bool temperature = Firebase.setString(fbdo, F("/dataInfo/temperature"), val.c_str());
      if(!temperature) {
        Serial.print("Failed to send temperature data: ");
        Serial.println(fbdo.errorReason());
      }

      if(btn.PressFood()) {
        bool badgeTime = Firebase.setString(fbdo, F("/dataInfo/timeFood"), Time.c_str());
        if(!badgeTime) {
          Serial.print("Failed to send time ");
          Serial.println(fbdo.errorReason());
        }
        Serial.println("[DEBUG] Food execute ");
        Serial.println(Time); //time pass here and badgeTime is not false but the problem was it was not able to send the Time value to my realtime db

        bool isFoodPress = Firebase.setString(fbdo, F("/dataInfo/food"),"Patient need food");
        if(!isFoodPress) {
          Serial.print("Failed to send food ");
          Serial.println(fbdo.errorReason());
        }
        delay(100);
      }
      if(btn.PressWater()) {
        bool badgeTime = Firebase.setString(fbdo, F("/dataInfo/timeWater"), Time.c_str());
        if(!badgeTime) {
          Serial.print("Failed to send time ");
          Serial.println(fbdo.errorReason());
        }
        Serial.println("[DEBUG] Water execute");
        bool isWaterPress = Firebase.setString(fbdo, F("/dataInfo/water"), "Patient need water!");
        if(!isWaterPress) {
          Serial.print("Failed to send water ");
          Serial.println(fbdo.errorReason());
        }
        delay(100);
      }

      if(btn.PressMedicine())
      {
        bool badgeTime = Firebase.setString(fbdo, F("/dataInfo/timeMed"), Time.c_str());
        if(!badgeTime) {
          Serial.print("Failed to send time ");
          Serial.println(fbdo.errorReason());
        }
        bool isMedPress = Firebase.setString(fbdo, F("/dataInfo/medicine"), "Patient need medicine!");
        if(!isMedPress) {
          Serial.print("Failed to send medecine ");
          Serial.println(fbdo.errorReason());
        }
        delay(100);
      }
    } else {
      Serial.println("Fail to send firebase!");
    }
  delay(500);
}
