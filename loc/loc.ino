#include <Arduino.h>

#include <ModbusMaster.h>

#include <esp_now.h>
#include <WiFi.h>

#include <ezButton.h>

#include <TFT_eSPI.h>
#include <SPI.h>


uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xCE, 0x50};
esp_now_peer_info_t peerInfo;

int packet;

int updateCode = 99999999; 
int stopCode = 88888888;

int limInfVel = 0;
int limSupVel = 4095;

int vel_VFD = 0;                
int waitStartVFD = 60;

int btn+ = 12;
int btn- = 13;
int btnPow = 14;


ModbusMaster node;

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex


TFT_eSPI tft = TFT_eSPI();

ezButton btnL(btn-);
ezButton btnR(btn+);
ezButton btnPWR(btnPow);


void tft_init();
void btnConfig();
void btnHandler();
void isr_btn+();
void isr_btn-();

void preTransmission();
void postTransmission();
void setupRS485();
uint8_t marchMotor(int speed);
uint8_t stopMotor();
uint8_t updateInfo();


// Callback when data is sent
void data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

// Callback when data is received
void data_receive(const uint8_t * mac, const uint8_t *incomingData, int len) {
        memcpy(&packet, incomingData, sizeof(packet));
        
        //change vfd velocity by modbus
        if(packet == updateCode){
                esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));
                 
        }

        if(packet == stopCode){
                vel_VFD = 0;
                print_vel(vel_VFD);
                stopMotor();
        }
       
        else{
                if(packet>limInfVel && packet<limSupVel){
                        vel_VFD = packet;
                        print_vel(vel_VFD);
                        marchMotor(vel_VFD);

                        //send acknowledgment to remote control
                        esp_now_send(broadcastAddress, (uint8_t *) &vel_VFD, sizeof(vel_VFD));
                      
                }
  }
  



  
 
void setup() {
                delay(waitStartVFD*1000); //wait to start VFD

                btnConfig();

                tft.begin();
        
                tft.setRotation(4);
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_WHITE);


                delay(1000);

                setupRS485();
  
  
                WiFi.mode(WIFI_STA);
                if (esp_now_init() != ESP_OK) {
                                return;
                }
                  
                esp_now_register_send_cb(data_sent);
                  
                  
                memcpy(peerInfo.peer_addr, broadcastAddress, 6);
                peerInfo.channel = 0;  
                peerInfo.encrypt = false;       
                  
                if (esp_now_add_peer(&peerInfo) != ESP_OK){
                                return;
                }

                  
                // Register for a callback function that will be called when data is received
                  esp_now_register_recv_cb(data_receive);
}



void loop() {
  

}


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

/* UI functions */

void btnConfig(){
        btnL.setDebounceTime(50);
        btnR.setDebounceTime(50);
        btnPWR.setDebounceTime(50);
        
        pinMode(btn+, INPUT_PULLUP);
        attachInterrupt(btn+, isr_btn+, FALLING);
        
        pinMode(btn-, INPUT_PULLUP);
        attachInterrupt(btn-, isr_btn-, FALLING);

        pinMode(btnPow, INPUT_PULLUP);
        attachInterrupt(btnPow, isr_btnPWR, FALLING);
}

void btnHandler(){
  
  
        btnL.loop();
        btnR.loop();
        btnPWR.loop();
        

        if (btnL.isPressed()) {
                vel_VFD -= vel_step;
               
                print_vel(vel_VFD);
                
                marchMotor(vel_VFD);
                 
        }

        if (btnR.isPressed()) {
                vel_VFD += vel_step;

                print_vel(vel_VFD);

                marchMotor(vel_VFD);
                 
        }

        if (btnPWR.isPressed()) {
                vel_VFD = 0;

                print_vel(vel_VFD);
                marchMotor(vel_VFD);
                 
        }

}


void isr_btn+(){
        count_timeout = 0;
  
}
void isr_btn-(){
        count_timeout = 0;
  
}

void isr_btnPWR(){
        count_timeout = 0;
}



void tft_init(){
        tft.begin();
        tft.setRotation(4);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(2);
}

void print_vel(int vel){
        tft.fillRect(0,50,200,70,TFT_DARKGREY);
        tft.setTextSize(4);
        tft.setTextColor(TFT_RED);
        tft.setCursor(30,70);
        tft.print(vel);
}