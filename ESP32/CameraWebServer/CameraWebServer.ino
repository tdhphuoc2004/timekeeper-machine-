#pragma once
#include "config.h"
#include "esp_camera.h"
#include <WiFi.h>
#define CAMERA_MODEL_AI_THINKER 
#include "camera_pins.h"
#include "UtilsWifi.h"
#include "ArduinoUtils.h"
#include "MqttUtils.h"
#include "FaceUtils.h"
#include <HardwareSerial.h>
HardwareSerial SerialPort(2); 

const char* faceServerUrl = "http://192.168.21.124:8080/recognize-face";
#define RXp2 15
#define TXp2 14

const char* mqtt_server = "192.168.21.124"; // e.g., "192.168.1.10" or "broker.example.com"
const int mqtt_port = 1883;
const char* mqtt_user = "user";
const char* mqtt_password = "123456";

// MQTT Topics
const char* faceTopic = "esp32cam/face"; // Topic to receive general messages
const char* faceSuccessTopic = "esp32cam/face_result";
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  //Serial1.begin(9600);    // Communication with Arduino
  // Serial.begin(115200);
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  // SerialPort.begin(115200, SERIAL_8N1,  RXp2, TXp2); // Serial1 for communication with Arduino
  // Serial.println("ESP32-CAM ready for bidirectional communication.");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 8;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 8;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_VGA;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_HVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif
//================================================= CODE HERE ========================================================
  // clearEEPROM(); 

  initializeWiFi();
   Serial.flush();

  // Serial.print("@");
  // Serial.end();
  // Serial.begin(115200);
}
int lastTime = millis();
void loop() 
{
  handleServerRequests();
  // mqtt_connect();
    char incomingChar = Serial.read();
    if (incomingChar == '@') 
    {
      // messageFromArduino.concat(incomingChar);
      String messageFromArduino = receiveFromArduino();
      sendDebugMessage(messageFromArduino);
      if(messageFromArduino == "Face") {
        incomingChar = Serial.read();
        while (incomingChar != '@') {
          incomingChar = Serial.read();
        }
        String photo = receiveFromArduino();
        String id = faceServerHandle();
        if (id != "NF" and id != "CAM") {
          String response = checkIn(id);
          sendToArduino(id);
          sendToArduino(response);
        } else if(id == "NF") {
          sendDebugMessage(photo);
          if(checkPhoto(photo)) {
            sendToArduino(id);
          } else {
            sendToArduino("Dark");
          }
          
        } else {
          sendToArduino(id);
        }
        
      } else {
        String response = checkIn(messageFromArduino);
        sendToArduino(response);
      }

    }
  delay(100); 
  
}