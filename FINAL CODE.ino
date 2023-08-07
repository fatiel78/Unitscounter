#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ctime>
#include <PubSubClient.h>
#include <FS.h>
#include <SPIFFS.h>

#include <Wire.h>  // Include the I2C library (required)
//#include <SparkFunSX1509.h>

long period;
unsigned long myperiod;
int delaysaccumulation = 0;
//SemaphoreHandle_t mqttSemaphore;

TaskHandle_t Task1;
//TaskHandle_t Task2;

//Variables for calculating the mean value
#define ARRAY_SIZE 30

int arr[ARRAY_SIZE];
int currentIndex = 0;
int thesize = 0;

////Configuration of SX1509
//const byte SX1509_ADDRESS = 0x3E; // SX1509 I2C address
int a = 0;
//SX1509 io;



//const byte SX1509_LASER_PIN = 3;

//to calculate the period between two passing units
unsigned long lastmillis1;
unsigned long lastmillis2;

//date and time of each passing unit

String formattedTime;

//parameters of the NTPclient server for getting date and time

const long utcOffsetInSeconds = 0;            // Change this based on your time zone
const unsigned int ntpUpdateInterval = 3600;  // Update NTP time every hour (in seconds)
WiFiUDP udp;
NTPClient ntpClient(udp, "pool.ntp.org", utcOffsetInSeconds);

bool wifiConnected = false;
bool ntpInitialized = false;
unsigned long lastNtpUpdate = 0;
unsigned long lastNtpEpochTime = 0;

//WIFI parameters
const char* ssid = "Fkdr";
const char* password = "fatielkhad12";


//MQTT parameters
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

char* mqttServer = "broker.emqx.io";
int mqttPort = 1883;


int mylinecount = 0;
char data[100];

//Number of calculated units
int e = 0;

//Configuring the interruption parameters
int s = 0;
int s1 = 0;

struct Laser {
  const uint8_t PIN;

  bool detected;
};

Laser laser = { GPIO_NUM_35, false };

void IRAM_ATTR isr() {

  laser.detected = true;
  s1++;
  if (s1 == 1) {
    if (a == 0) {
      lastmillis1 = millis();
      a++;
    } else {
      lastmillis2 = millis();
      a = 0;
    }
  }

  if (s1 == 2) { s1 = 0; }
}

//used functions
void shiftAndAppend(int arr[], int size, int element) {
  // Shift elements to the left
  for (int i = 0; i < size - 1; i++) {
    arr[i] = arr[i + 1];
  }

  // Append the new element at the end
  arr[size - 1] = element;
}



void appendToStaticArray(int arr[], unsigned long elementToAdd) {
  static int currentIndex = 0;  // Static variable to keep track of the current index

  // Check if the array is already full
  if (currentIndex >= ARRAY_SIZE) {
    //  printf("Static array is full. Unable to append the element.\n");
    return;
  }

  // Append the new element at the end
  arr[currentIndex] = elementToAdd;
  currentIndex++;
}


void updateNTPTime() {
  ntpClient.update();
  if (ntpClient.getSeconds() != 0) {
    lastNtpEpochTime = ntpClient.getEpochTime();
    lastNtpUpdate = millis();
    Serial.println("NTP time updated.");
  } else {
    Serial.println("Failed to get NTP time!");
  }
}

//String formatTime(unsigned long epochTime) {
//  time_t time = epochTime;
//  struct tm *timeinfo;
//  timeinfo = localtime(&time);
//
//  char buffer[30];
//  sprintf(buffer, "%02d-%02d-%04d, %02d:%02d:%02d",
//          timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
//          timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
//  return String(buffer);
//}

String formatTime(unsigned long epochTime) {
  time_t time = epochTime;
  struct tm* timeinfo;
  timeinfo = localtime(&time);

  char buffer[40];  // Increase buffer size to accommodate milliseconds
  sprintf(buffer, "%02d-%02d-%04d, %02d:%02d:%02d.%03d",
          timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
          timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
          millis() % 1000);  // Add milliseconds

  return String(buffer);
}


int countLinesInFile(File file) {
  int lineCount = 0;
  while (file.available()) {
    if (file.readStringUntil('\n').length() > 0) {
      lineCount++;
    }
  }
  return lineCount;
}

void saveMqttMessage(String message) {
  int lineCount = 0;

  File file = SPIFFS.open("/mqtt_messages.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Get the current line count
  lineCount = countLinesInFile(file);

  // Close the file
  file.close();

  // Open the file for appending
  file = SPIFFS.open("/mqtt_messages.txt", FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  // Append the message to the file
  file.println(message);
  Serial.println("Saved MQTT message: " + message);

  // Increase the line count
  lineCount++;
  mylinecount = lineCount;

  // Optionally print the number of lines in the file
  Serial.println("Line count in file: " + String(lineCount));


  file.close();
}


void publishStoredMessages() {
  // Open the file for reading
  File file = SPIFFS.open("/mqtt_messages.txt");

  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Read and publish stored messages line by line
  while (file.available()) {
    String message = file.readStringUntil('\n');




    // Publish the stored message to the MQTT broker
    //sprintf(data, "%s", String.toCharArray(message));
    // Serial.println(data);
    mqttClient.publish("/dari/datetime", message.c_str());
    //mqttClient.publish("/test/datetime", data);
    Serial.println("Published: " + message);

    // Uncomment the line below if you want to add a small delay between publishing messages
    // delay(100);

    // OPTIONAL: Remove the line from storage (only if you want to delete the messages after publishing)
    // file.seek(file.position());
    //file.write('\0'); // Overwrite the line with null character (optional: removes the line content)
  }

  file.close();

  // OPTIONAL: Delete the file after publishing all the messages (only if you want to delete the messages after publishing)
  SPIFFS.remove("/mqtt_messages.txt");
  mylinecount = 0;
}




void connectToMqtt() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      // Check for stored messages and publish them
      publishStoredMessages();
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void reconnectToMqtt() {
  if (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      // Check for stored messages and publish them
      int t1 = millis();
      publishStoredMessages();
      int t2 = millis();
      Serial.println("period :" + String(t2 - t1));
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqttClient.state());
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Callback - ");
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

void setupMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  // set the callback function
  mqttClient.setCallback(callback);
}


//Task1 to publish the messages
void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());


  while (1) {
    // Publish MQTT messages if there are any stored
    if (mylinecount >= 20 && mqttClient.connected()) {
      int t1 = millis();
      publishStoredMessages();
      int t2 = millis();
      Serial.println("period :" + String(t2 - t1));
      mylinecount = 0;
    }

    // Delay before the next iteration
    vTaskDelay(pdMS_TO_TICKS(100));  // You can adjust the delay duration if needed
  }
}




bool isFileEmpty(const String& filename) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  // Get the size of the file
  size_t fileSize = file.size();

  // Close the file
  file.close();

  // Check if the file is empty (size is zero)
  return fileSize == 0;
}

void setup() {
  Serial.begin(115200);
  // mqttSemaphore = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    8192,      /* Stack size of task */
    NULL,      /* parameter of the task */
    2,         /* priority of the task */
    NULL,      /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

  
  //SX1509 SETUP
  //    Wire.begin(); //Initialize I2C bus
  //    io.begin();
  //
  //      if (io.begin(SX1509_ADDRESS) == false)
  //  {
  //    Serial.println("Failed to communicate. Check wiring and address of SX1509.");
  //
  //    while (1)
  //      ; // If we fail to communicate, loop forever.
  //  }

  //SETTING THE PINS UP AND ATTACHING INTERRUPTIONS
  pinMode(GPIO_NUM_35, INPUT);
  //   io.enableInterrupt(SX1509_LASER_PIN, FALLING);
  //   io.pinMode(SX1509_LASER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GPIO_NUM_35), isr, RISING);


  //SETTING UP THE WIFI
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Wi-Fi connected successfully");
  wifiConnected = true;

  //SETTING UP MQTT
  setupMQTT();
  connectToMqtt();

  //SETTING UP SPIFFS
  if (!SPIFFS.begin(false)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //SETTING UP NTPCLIENT SERVER

  udp.begin(123);
  ntpClient.begin();
  ntpClient.update();  // Initial update
  if (ntpClient.getSeconds() == 0) {
    Serial.println("Failed to get NTP time!");
    ntpInitialized = false;
  } else {
    lastNtpEpochTime = ntpClient.getEpochTime();
    lastNtpUpdate = millis();
    ntpInitialized = true;
  }

  //SETTING UP THE FILE TO WHICH WE WILL APPEND THE MQTT MESSAGES
  File fileToWrite = SPIFFS.open("/mqtt_messages.txt", FILE_WRITE);
  if (!fileToWrite) {
    Serial.println("There was an error opening the file for writing");
    return;
  }

  fileToWrite.close();


  if (SPIFFS.exists("/mqtt_messages.txt")) {
    Serial.println("FILE EXISTS");
  } else {
    Serial.println("FILE DOESNT EXIST");
  }
}


void loop() {

  //READING THE SENSOR'S PIN
  int lp = digitalRead(GPIO_NUM_35);



  // IF AN INTERRUPTION HAS OCCURED

  if (laser.detected) {

    //because
    s++;

    if (s == 1) {
      e++;

      if (wifiConnected) {

        if (mylinecount < 20 || !mqttClient.connected()) {

          if (millis() - lastNtpUpdate >= ntpUpdateInterval * 1000) {
            updateNTPTime();
          }
          if (ntpInitialized) {
            String formattedTime = formatTime(ntpClient.getEpochTime());
            saveMqttMessage(formattedTime);
            Serial.println(formattedTime);
          }
        }
      }


      else {
        unsigned long currentMillis = millis();
        unsigned long secondsPassed = (currentMillis - lastNtpUpdate) / 1000;
        unsigned long calculatedEpochTime = lastNtpEpochTime + secondsPassed;
        String formattedTime = formatTime(calculatedEpochTime);
        saveMqttMessage(formattedTime);
        Serial.println(formattedTime);
      }


      //CALCULATING THE PERIOD

      period = lastmillis2 - lastmillis1;

      if (period >= 0) {
        myperiod = period;
      } else {
        myperiod = period * (-1);
      }

      if (myperiod >= 120000) {
        delaysaccumulation += myperiod;
        sprintf(data, "%i", delaysaccumulation);
        // Serial.println(data);
        mqttClient.publish("/dari/retard", data);
      }

      if (e < 30) {                          // if units are less than 30 then we will just add them to the array
        appendToStaticArray(arr, myperiod);  //ADDING THE PERIOD TO THE ARRAY
      }

      else {  // when the units pass 30 we will start shifting and appending (like a ring buffer)

        shiftAndAppend(arr, ARRAY_SIZE, myperiod);  // ADDING THE LAST PERIOD TO THE ARRAY

        //CALCULATING THE MEAN OF THE LAST 30 PERIODS (STATISTICS TO KNOW IF THERE IS A DELAY)
        int sum = 0;
        for (int i = 0; i < ARRAY_SIZE; i++) {

          sum += arr[i];
        }

        float meanvalue1 = sum / ARRAY_SIZE;
        int meanvalue = (int)meanvalue1;

        String meanString = "mean value : " + String(meanvalue);

        // Printing the concatenated string to the serial port
        //Serial.println(meanString);


        //sending the mean value via MQTT

        sprintf(data, "%i", meanvalue);
        // Serial.println(data);
        mqttClient.publish("/dari/mean", data);
      }
    }

    if (s == 2) { s = 0; }

    laser.detected = false;
  }



  sprintf(data, "%i", lp);
  //Serial.println(data);
  mqttClient.publish("/dari/lp1", data);

  sprintf(data, "%i", e);
  // Serial.println(data);
  mqttClient.publish("/dari/nb", data);

  String per = String(myperiod) + " ms";

  sprintf(data, "%i", myperiod);
  // Serial.println(data);
  mqttClient.publish("/dari/period", data);

  if (!mqttClient.connected()) {
    Serial.println("mqtt disconnected");
    reconnectToMqtt();
  }



  delay(10);
}
