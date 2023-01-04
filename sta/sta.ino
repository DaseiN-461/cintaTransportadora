
/////////This is a station device that control a variable veloctity  by esp-now.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////



#include <esp_now.h>
#include <WiFi.h>




uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xCE, 0x50};
esp_now_peer_info_t peerInfo;

int packet;

String success;

int vel_VFD = 50;



// Callback when data is sent
void data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void data_receive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&packet, incomingData, sizeof(packet));
  Serial.print("packet received: [");
  Serial.print(packet);
  Serial.println("]");
  

  //change vfd velocity by modbus
  
  if(packet == 99999999){
    Serial.println("Updating remote control");
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));
     
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }else{
    vel_VFD = packet;
    esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));    
  }
  
}
 
void setup() {

  Serial.begin(115200);
  
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(data_sent);
  
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;       
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(data_receive);
  Serial.println("Started sucessfully");
  Serial.println("Waiting for a packet ...");
}



void loop() {
  

}
