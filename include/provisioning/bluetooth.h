#include <WiFi.h>
#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config/device_config.h"
#include "config/flash_config.h"

bool checkBluetoothStatus();
void configMode();
void resetDevice();
void bootButtonListener();
void connectToWiFi(const String &ssid, const String &password);
void listenInputButtons_withoutInternet();

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
String receivedData = "";
bool credentialsReceived = false;
bool configModeActive = false;
bool deviceConnected = false;

// BLE Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("BLE Device connected");
  }

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("BLE Device disconnected");
    BLEDevice::startAdvertising(); // Restart advertising
  }
};

// BLE Characteristic Callbacks
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      receivedData = "";
      for (int i = 0; i < value.length(); i++)
      {
        char c = (char)value[i];
        if (c == '\n')
        {
          int separatorIndex = receivedData.indexOf(',');
          if (separatorIndex != -1)
          {
            String ssid = receivedData.substring(0, separatorIndex);
            String password = receivedData.substring(separatorIndex + 1);

            Serial.printf("✔ Received SSID: %s Password: %s\n", ssid.c_str(), password.c_str());

            setSSID(ssid);
            setPassword(password);
            Serial.println("✔ SSID & Password Received & Set Successfully...");

            // Connect to WiFi
            WiFi.begin(ssid.c_str(), password.c_str());

            // Send DID and its structure TODO
            String didStructure = DID;
            pCharacteristic->setValue(didStructure.c_str());
            pCharacteristic->notify();
            Serial.println("✔ Device ID\n" + didStructure + " \nSent Successfully...");

            credentialsReceived = true;
            configModeActive = false;
          }
        }
        else
        {
          receivedData += c;
        }
      }
    }
  }
};

bool checkBluetoothStatus()
{
  // In BLE, check if the controller is initialized and enabled
  if (BLEDevice::getInitialized())
  {
    return true; // BLE is enabled
  }
  else
  {
    return false; // BLE is disabled
  }
}

void configMode()
{
  Serial.println("Entering Config Mode...");
  configModeActive = true;
  credentialsReceived = false;
  receivedData = "";

  // Initialize BLE
  BLEDevice::init(BLEid);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("✔ BLE Advertising Started...");

  // Wait for credentials
  while (configModeActive)
  {
    if (credentialsReceived)
    {
      // Stop BLE
      pServer->removeService(pService);
      BLEDevice::deinit(true);
      Serial.println("✔ BLE Ended Successfully...");
      configModeActive = false;
    }
    delay(100); // Prevent tight loop
  }

  Serial.println("Exiting Config Mode...");
  ESP.restart();
}

void resetDevice()
{
  Serial.println("Resetting device credentials...");
  preferences.clear();
  preferences.end();
  delay(500);
  ESP.restart();
}

void bootButtonListener()
{
  if (digitalRead(BOOT_BUTTON) == LOW)
  {
    unsigned long pressStartTime = millis();
    while (digitalRead(BOOT_BUTTON) == LOW)
    {
      if (millis() - pressStartTime >= 3000)  // hold for 3 seconds
      {
        configMode();
        Serial.println("Boot button pressed for 3 seconds, entering config mode...");
        return;
      }
      delay(10); 
    }
  }
}


void connectToWiFi(const String &ssid, const String &password)
{
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        bootButtonListener();
        listenInputButtons_withoutInternet();
        delay(100);
    }

    Serial.println("\n✔️  Connected to WiFi -> IP: ");
    Serial.println(WiFi.localIP());
}

void listenInputButtons_withoutInternet()
{
  static bool lastStateIN[10] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

  for (int i = 0; i < NODE_COUNT; i++)
  {
    bool currentState = digitalRead(inputPins[i]);
    if (currentState != lastStateIN[i])
    {
      String nodeId = nodes[i];
      bool currentStatus = getLightStatus(nodeId);
      bool newStatus = !currentStatus;
      setLightStatus(nodeId, newStatus);
      digitalWrite(outputPins[i], newStatus ? HIGH : LOW);
      Serial.printf("[Action]: 🟢 Light %s toggled to %s\n", nodeId.c_str(), newStatus ? "ON" : "OFF");
      lastStateIN[i] = currentState;
    }
  }
}
