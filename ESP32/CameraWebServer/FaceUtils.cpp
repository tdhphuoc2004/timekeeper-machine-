#include "FaceUtils.h"

String faceServerHandle() {
  bool face_success = false;
  int startTime = millis();
  String id = "";
  while(!face_success) {
    String boundary = "----ESP32Boundary";
    String payload = "--" + boundary + "\r\n";
    payload += "Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\n";
    payload += "Content-Type: image/jpeg\r\n\r\n";

    WiFiClient client;
    HTTPClient http;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return "CAM";
    }

    // Create the HTTP payload
    http.begin(client, faceServerUrl);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    int httpResponseCode = http.POST(payload + String((char *)fb->buf, fb->len) + "\r\n--" + boundary + "--\r\n");

    if (httpResponseCode == 200) {
        // Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        String response = http.getString();
        response.replace("\"", "");
        // Serial.println("Server response: " + response);
        sendDebugMessage(response);

        if(response != "Unknown") {
          
          face_success = true;
          id = response;
          esp_camera_fb_return(fb);
          http.end();
          break;
        }
    }

    esp_camera_fb_return(fb);
    http.end();
    if(millis() - startTime > 5000) break;
    
    delay(500);
  }
  if(face_success) return id;
  else return "NF";
}