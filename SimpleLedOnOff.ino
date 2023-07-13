#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

const int LIGHT_PIN = 4;

const char* ssid     = "MyESP32CAM";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <script>
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCarInput;
      
      function initCarInputWebSocket() 
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event){};
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};        
      }
      
      function initWebSocket() 
      {
        initCarInputWebSocket();
      }

      function sendButtonInput(value) 
      {
        websocketCarInput.send(value);
      }
    
      window.onload = initWebSocket;
    </script>
  </head>
  <body align="center" style="background-color:white">
    <h2 style="color: teal;text-align:center;">LED Control</h2>
    <button style="font-size:20px;padding:10px 20px;margin:10px;" onclick="sendButtonInput('on')">Turn On</button>
    <button style="font-size:20px;padding:10px 20px;margin:10px;" onclick="sendButtonInput('off')">Turn Off</button>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server, 
                              AsyncWebSocketClient *client, 
                              AwsEventType type,
                              void *arg, 
                              uint8_t *data, 
                              size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        String message = String((char *)data);
        Serial.println("Received message: " + message);
        
        if (message == "on") {
          digitalWrite(LIGHT_PIN, HIGH); // Turn on the LED
        } else if (message == "off") {
          digitalWrite(LIGHT_PIN, LOW); // Turn off the LED
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setup(void) 
{
  pinMode(LIGHT_PIN, OUTPUT);    
  digitalWrite(LIGHT_PIN, LOW); // Initially turn off the LED
  
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() 
{
  wsCarInput.cleanupClients(); 
}
