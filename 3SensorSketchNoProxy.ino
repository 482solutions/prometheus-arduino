#include <WiFiEsp.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <HX711.h>
#include <Ethernet.h>
#include "SoftwareSerial.h"

#define RST_PIN 9
#define SS_PIN 10

const byte ROWS = 3;
const byte COLS = 3;

char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'}};

const byte humiditySensorPin = 16;
const byte rowPins[ROWS] = {4, 3, 2};
const byte colPins[COLS] = {7, 6, 5};

const String accessPassword = "1234";
const String PID = "D5 50 7E 63";
const char ssid[] = "AndroidAP";
const char wifiPassword[] = "lfwc7073";
const byte port = 3000;

SoftwareSerial Serial1(A3, A4);
WiFiEspClient client;
MFRC522 mfrc522(SS_PIN, RST_PIN);
Keypad keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
StaticJsonBuffer<100> jsonBuffer;
HX711 scale(A1, A0);

short status = WL_IDLE_STATUS;
byte sensorsState[3] = {0, 0, 0};
short weight;
char key = 0;
String password;
boolean isInit = false;

void setup()
{
  Serial.begin(19200);
  Serial1.begin(115200);
  Serial.println("Starting serial communication...");

  WiFi.init(&Serial1);

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, wifiPassword);
  }

  printWifiStatus();

  scale.tare();
  scale.set_scale(-14.77);

  keypad.setHoldTime(10);

  SPI.begin();
  Serial.println("Starting SPI bus");

  mfrc522.PCD_Init();

  pinMode(humiditySensorPin, INPUT);
}

void loop()
{
  if (checkAllSensorsState())
  {
    isInit = false;
    resetAllSensorsState();
  }

  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    Serial.println(key);
    password += key;
  }

  if (password.length() >= 4)
  {
    if (accessPassword.equals(password))
    {
      sendRequest(createRequestBody(1, "button", true));
      isInit = true;
      password = "";
    }
    else
    {
      sendRequest(createRequestBody(0, "button", "Wrong Password", true));
      password = "";
    }
  }
  weight = scale.get_units(10) * 0.035274;
  if (weight<300 + 10 & weight> 300 - 10)
  {
    Serial.println("WeightSensorState: " + weight);
    if (sensorsState[0] == 1)
    {
      sendRequest(createRequestBody(0, "WeightSensor", "Already signed", false));
    }
    else
    {
      sensorsState[0] = 1;
      sendRequest(createRequestBody(1, "WeightSensor", false));
    }
    while (weight<(300 + 10) & weight> 300 - 10)
    {
      weight = scale.get_units(10) * 0.035274;
    }
  }

  if (digitalRead(humiditySensorPin) == LOW)
  {
    Serial.println("HumiditySensorState: LOW");
    if (sensorsState[1] == 1)
    {
      sendRequest(createRequestBody(0, "HumiditySensor", "Already signed", false));
    }
    else
    {
      sensorsState[1] = 1;
      sendRequest(createRequestBody(1, "HumiditySensor", false));
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    delay(50);
    return;
  }

  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == PID)
  {
    if (sensorsState[2] == 1)
    {
      sendRequest(createRequestBody(0, "RFIDMagnet", "Already signed", false));
    }
    else
    {
      sensorsState[2] = 1;
      sendRequest(createRequestBody(1, "RFIDMagnet", false));
    }
  }
  else
  {
    Serial.println("Access denied");
    sendRequest(createRequestBody(0, "RFIDMagnet", "Wrong card with PID: " + content.substring(1), false));
  }
}

JsonObject &createRequestBody(int value, String sensorId, boolean primary)
{
  return createRequestBody(value, sensorId, "", primary);
}

JsonObject &createRequestBody(int value, String sensorId, String message, boolean primary)
{
  jsonBuffer.clear();
  JsonObject &root = jsonBuffer.createObject();
  root["sensorId"] = sensorId;
  root["deviceId"] = "arduinoUNO";
  root["shipmentId"] = "shrimp-25-05-18";

  if (!isInit & !primary)
  {
    root["status"] = "Transaction is not initiated";
    root["value"] = 0;
    Serial.println("Hu1");
  }
  else if (message != "")
  {
    root["status"] = message;
    root["value"] = 0;
    Serial.println("Hu2");
  }
  else
  {
    root["status"] = "Success";
    root["value"] = value;
    Serial.println("Hu3");
  }
  return root;
}

boolean checkAllSensorsState()
{
  boolean result = true;
  for (byte i = 0; i < sizeof(sensorsState); i++)
  {
    if (sensorsState[i] == 0)
    {
      result = false;
    }
  }
  return result;
}

void resetAllSensorsState()
{
  for (byte i = 0; i < sizeof(sensorsState); i++)
  {
    sensorsState[i] = 0;
  }
}

void sendRequest(JsonObject &root)
{
  Serial.println("Creating request");
  client.stop();
  root.prettyPrintTo(Serial);
  Serial.println();

  if (client.connect("46.101.30.8", 3000))
  {
    if (root["sensorId"] == "button")
    {
      client.println("POST /buyer/prepay HTTP/1.1");
    }
    else
    {
      client.println("POST /device/signal HTTP/1.1");
    }
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(root.measurePrettyLength());
    client.println();
    root.prettyPrintTo(client);
    client.println();
  }
  else
  {
    Serial.println("Connection failed");
  }
}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}