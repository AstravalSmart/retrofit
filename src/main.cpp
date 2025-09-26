#include "cloud_comm/cloud_comm.h"
#include "provisioning/bluetooth.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("----------------------------");
  Serial.println("Setup START...");

  pinModeSetterForDevice();
  initPreferences();

  String ssid = getSSID();
  String password = getPassword();

  if (ssid != "" && password != "")
  {
    connectToWiFi(ssid.c_str(), password.c_str());
  }
  else
  {
    configMode();
  }

  firebaseSetup();

  Serial.println("----------------------------");
}

void loop()
{
  delay(100);
  app.loop();
  Database.loop();
  bootButtonListener();
  updateStateValues();
  listenInputButtons();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected, restarting...");
    delay(1000);
    ESP.restart();
  }
}
