
/////////This is a station device that control a variable veloctity  by esp-now.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

#include <Wire.h>
#include <ModbusMaster.h>

#include <esp_now.h>
#include <WiFi.h>




uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xCE, 0x50};
esp_now_peer_info_t peerInfo;

int packet;

int updateCode = 99999999; 
int stopCode = 88888888;

int limInfVel = 0;
int limSupVel = 4095;


String success;

int vel_VFD = 0;

int waitStartVFD = 60;

ModbusMaster node;

void preTransmission();
void postTransmission();
void setupRS485();
uint8_t marchMotor(int speed);
uint8_t stopMotor();
uint8_t updateInfo();


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
  
  if(packet == updateCode){
          Serial.println("Updating remote control");
          esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));
           
          if (result == ESP_OK) {
                  Serial.println("Sent with success");
          }
          else {
                  Serial.println("Error sending the data");
          }
  }

  if(packet == stopCode){
          uint8_t mb_error = stopMotor();
  }
 
  else{
          if(packet>limInfVel && packet<limSupVel){
                  vel_VFD = packet;
                  uint8_t mb_error = marchMotor(vel_VFD);
                  esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));
          }
  }
  



  
 
void setup() {
  delay(waitStartVFD*1000); //wait to start VFD
  //comment every Serial if test with max485 converter or use other uart port
  //Serial.begin(115200);
  setupRS485();
  
  
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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
/* Modbus RTU functions */

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
  delay(10);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  delay(10);
}

void setupRS485() {
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial.begin(9600, SERIAL_8E1);
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

uint8_t marchMotor(int speed) {
    uint8_t result;
    if(speed > 0) {
        node.writeSingleRegister(0x0063, 0x47F); //clock sense
    } else if (speed < 0) {
        node.writeSingleRegister(0x0063, 0xC7F); // inverse clock sense
    }
    result = node.writeSingleRegister(0x0064, map(abs(speed), 0, 100, 0, 16383));
    return result;
}

uint8_t stopMotor() {
    uint8_t result;
    node.writeSingleRegister(0x0064, 0);
    result = node.writeSingleRegister(0x0063, 0x047E);
    return result;
}

  
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////