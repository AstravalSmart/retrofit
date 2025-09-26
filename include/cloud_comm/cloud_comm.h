#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <FirebaseClient.h>
#include "config/firebase_config.h"
#include "config/device_config.h"
#include "config/flash_config.h"
#include "ExampleFunctions.h"

void processData(AsyncResult &aResult);
void setOnline();
void setLightStatus(String nodeId, String statusPath, bool val);

SSL_CLIENT ssl_client, stream_ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000);
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult databaseResult;

unsigned long ms = 0;


void initFirebaseApp()
{
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

  ssl_client.setInsecure();
  stream_ssl_client.setInsecure();


  Serial.println("Initializing app...");
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");

  app.getApp<RealtimeDatabase>(Database);

  Database.url(DATABASE_URL);

  Serial.println("✔️  Firebase App initialized Successfully...");
}

void startFirebaseStream()
{
  streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
  Database.get(streamClient, "devices/" + String(DID) + "/node", processData, true, "streamTask");
  Serial.println("️‍🔥▶ Firebase Stream Started Successfully...");
}

bool firstTime = false;
void updateStateValues()
{
    if (app.ready() && !firstTime)
    {
        for (int i = 0; i < NODE_COUNT; i++)
        {
            String nodeId = nodes[i];
            bool status = getLightStatus(nodeId);
            Serial.printf("📌Light %s: %s\n", nodeId.c_str(), status ? "ON" : "OFF");
        }

        for (int i = 0; i < NODE_COUNT; i++)
        {
          String nodeId = nodes[i];
          bool status = getLightStatus(nodeId);
          String statusPath = "/devices/" + String(DID) + "/node/" + nodeId;
          Database.set<bool>(aClient, statusPath + "/cmd", status, processData, "setStatusTask_" + nodeId);
          Database.set<bool>(aClient, statusPath + "/status", status, processData, "setStatusTask_" + nodeId);
          Serial.printf("✔️  Setting %s/status to %s in Firebase\n", statusPath.c_str(), status ? "true" : "false");
        }
        
        firstTime = true;
        setOnline();
        Serial.println("----------------------------");
    }
}

void setOnline()
{
  String path = "/devices/" + String(DID);
  Database.set<bool>(aClient, path + "/state", true);
  Serial.printf("✔️  Device %s is online\n", DID.c_str());
}

void processData(AsyncResult &aResult)
{
  if (!aResult.isResult())
    return;

  // if (aResult.isEvent())
  // {
  //   Firebase.printf("🔥 Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  // }

  //   if (aResult.isDebug())
  //   {
  //     Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  //   }

  //   if (aResult.isError())
  //   {
  //     Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  //   }

  if (aResult.available())
  {
    String payload = aResult.c_str();

    if (payload.indexOf("event: keep-alive") >= 0)
      return;

    // Firebase.printf("🔥 task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());

    // Check if the event is "put" or "patch" and contains "cmd"
    if ((payload.indexOf("event: put") >= 0 || payload.indexOf("event: patch") >= 0) && payload.indexOf("\"cmd\":") >= 0)
    {
      // Extract the path
      int pathStart = payload.indexOf("\"path\":\"") + 8;
      int pathEnd = payload.indexOf("\"", pathStart);
      String path = payload.substring(pathStart, pathEnd);

      // Only process paths like "/n1", "/n3", etc., and skip root path "/"
      if (path.length() > 1 && path.startsWith("/"))
      {
        // Extract node ID (e.g., "n1" from "/n1")
        String nodeId = path.substring(1);
        if (nodeId.length() > 0)
        {
          // Extract the cmd value
          int cmdStart = payload.indexOf("\"cmd\":") + 6;
          int cmdEnd = payload.indexOf(",", cmdStart);
          if (cmdEnd == -1)
            cmdEnd = payload.indexOf("}", cmdStart);
          String cmdStr = payload.substring(cmdStart, cmdEnd);
          bool cmdValue = (cmdStr.indexOf("false") >= 0) ? false : true;
          // Serial.printf("Detected change in %s/cmd: %d\n", nodeId.c_str(), cmdValue);

          // Construct the status path (e.g., "/devices/" + String(DID) + "/node/n1")
          String statusPath = "/devices/" + String(DID) + "/node/" + nodeId;

          // Set light status for the node
          setLightStatus(nodeId, statusPath, cmdValue);

          // Serial.printf("Updating %s/status...\n", statusPath.c_str());
        }
      }
    }
  }
}

void firebaseSetup()
{
  initFirebaseApp();
  startFirebaseStream();
}

// General function to set light status
object_t json;
JsonWriter writer;
void setLightStatus(String nodeId, String statusPath, bool val)
{
    int index = -1;
    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (nodes[i] == nodeId)
        {
            index = i;
            break;
        }
    }

    if (index != -1)
    {
        int pin = outputPins[index];
        Serial.printf("[Action]: 🟢 Turn the light %s - %s\n", nodeId.c_str(), (val ? "ON" : "OFF"));
        digitalWrite(pin, val ? HIGH : LOW);

        setLightStatus(nodeId, val);  // update local storage

        writer.create(json, "status", val);
        Database.update(aClient, statusPath, json, processData, "updateStatusTask");
    }
    else
    {
        Serial.printf("[Error]: 🔴 No pin mapped for node %s\n", nodeId.c_str());
    }
}


void listenInputButtons()
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
      String statusPath = "/devices/" + String(DID) + "/node/" + nodeId;
      setLightStatus(nodeId, statusPath, newStatus);
      Database.set<bool>(aClient, statusPath + "/cmd", newStatus, processData, "setCmdTask_" + nodeId);
      Serial.printf("[Cloud]:🔥 Node %s - %s\n",nodeId, newStatus ? "true" : "false");
      lastStateIN[i] = currentState;
    }
  }
}

