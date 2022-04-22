#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncElegantOTA.h>

#define LED_BUILTIN 7
#define WIFI_RETRY_TIMER 10

// Set these to your desired credentials.
const char *ap_ssid = "test_wifi_network";
const char *ap_password = "password";
const char* host = "esp32";

AsyncWebServer server(80);

Preferences preferences;

bool wifi_connected = false;

String error_message = "";

const char* serverIndex =
 "<form action=\"/H\">Click <input type=\"submit\" value=\"here\"> to turn ON the LED.</form><br>"
 "<form action=\"/L\">Click <input type=\"submit\" value=\"here\"> to turn OFF the LED.</form><br>"
 "<br>"
 "<br>"
 "<div>Change the Wifi network:</div><br>"
 "<form action=\"/WIFI\">"
     "SSID: <input type=\"text\" name=\"ssid\">"
     "<br>"
     "Password: <input type=\"text\" name=\"password\">"
     "<br>"
     "<input type=\"submit\" value=\"Connect\">"
 "</form>";

void notFound(AsyncWebServerRequest *request) {
  request->send(404);
}

void configAP() {
  Serial.println();
  Serial.println("Configuring access point...");

  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void connectWifi() {

    preferences.begin("wifi-data", true);
    
    String wifi_ssid = preferences.getString("ssid", "");
  
    //If there is a saved wifi network, attempt to connect to it
    if (wifi_ssid != "") {
      String wifi_password = preferences.getString("password", "");
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(wifi_ssid);
  
      WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
      int retryCount = 0;
      while (WiFi.status() != WL_CONNECTED && retryCount != WIFI_RETRY_TIMER) {
          delay(1000);
          Serial.print(".");
          retryCount++;
      }
  
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        wifi_connected = false;
        error_message = "Could not connect to saved Wifi network " + wifi_ssid;
        return;
      }
  
      wifi_connected = true;
      Serial.println("");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  
    preferences.end();
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(final){
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.println("Configuring LED output...");
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Done");
  Serial.println();

  connectWifi();

  //If the wifi connection fails, run in AP mode
  if (not wifi_connected) {
    configAP();
  }

  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", serverIndex);
  });

  server.on("/H", HTTP_GET, [] (AsyncWebServerRequest *request) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Turned on the LED");
    request->send(200, "text/html", String("Turing LED on!<br><br>") + serverIndex);
  });

  server.on("/L", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Turned off the LED");
    digitalWrite(LED_BUILTIN, LOW);
    request->send(200, "text/html", String("Turing LED off!<br><br>") + serverIndex);
  });

  server.on("/WIFI", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String new_ssid = "";
    String new_password = "";
    String error_message = "";
    // GET values on <ESP_IP>/wifi?ssid=<new_ssid>&password=<new_password>
    if (request->hasParam("ssid") and request->hasParam("password")) {
      new_ssid = request->getParam("ssid")->value();
      new_password = request->getParam("password")->value();
      if (new_ssid.length() < 33 and new_password.length() < 64) {
        Serial.println();
        Serial.println("Saving new wifi data");
        preferences.begin("wifi-data", false);
        preferences.putString("ssid", new_ssid);
        preferences.putString("password", new_password);
        preferences.end();
        Serial.println("Redirecting");
        request->redirect("/");
        Serial.println("Delaying 5 seconds");
        delay(5000);
        Serial.println("Restarting...");
        ESP.restart();
      }
      else {
        error_message = "SSID or password is too long";
      }
    }
    else {
      error_message = "Missing SSID or Password";
    }

    request->send(200, "text/html", error_message + "<br><br>" + serverIndex);
  });

  AsyncElegantOTA.begin(&server);
  server.begin();
  server.onNotFound(notFound);
  Serial.print("Server started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }
}
