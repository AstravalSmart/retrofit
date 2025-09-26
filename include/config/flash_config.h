#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include <Preferences.h>

Preferences preferences;

void initPreferences()
{
    preferences.begin("wifiCreds", false);
    preferences.begin("deviceStatus", false);
}

String getSSID()
{
    preferences.begin("wifiCreds", false);
    String ssid = preferences.getString("ssid", "");
    preferences.end();
    return ssid;
}

String getPassword()
{
    preferences.begin("wifiCreds", false);
    String password = preferences.getString("password", "");
    preferences.end();
    return password;
}

void setSSID(String ssid)
{
    preferences.begin("wifiCreds", false);
    preferences.putString("ssid", ssid);
    preferences.end();
}

void setPassword(String password)
{
    preferences.begin("wifiCreds", false);
    preferences.putString("password", password);
    preferences.end();
}

void setLightStatus(String nodeId, bool status)
{
    preferences.begin("deviceStatus", false);
    preferences.putBool(nodeId.c_str(), status);
    preferences.end();
}

bool getLightStatus(String nodeId)
{
    preferences.begin("deviceStatus", false);
    bool status = preferences.getBool(nodeId.c_str(), false); // Default to false if not found
    preferences.end();
    return status;
}

#endif