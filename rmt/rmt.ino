#include <Arduino.h>


#include <esp_now.h>
 
#include <WiFi.h>

#include <ezButton.h>

#include <TFT_eSPI.h>
#include <SPI.h>


uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xC1, 0xFC, 0x30};
 
bool update_flag = false;
bool conection_flag = false;


int packet;
int request;


esp_now_peer_info_t peerInfo;


int btn1 = 0;
int btn2 = 35;

int vel_step = 5;
int vel_VFD;
int vel_current;

int limInfVel = 0;
int limSupVel = 4095;

int count_timeout = 0;
int timeout = 10;


#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex


TFT_eSPI tft = TFT_eSPI();

ezButton btnL(btn1);
ezButton btnR(btn2);


void tft_init();
void espNow_begin();
void print_vel(int vel);

void btnConfig();
void btnHandler();
void isr_btn1();
void isr_btn2();

void update_velocity();
void update_verification();


void data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
        if (status ==0){
                conection_flag = true;
        }
        else{
                conection_flag = false;
        }

        update_flag = false;
}

void data_receive(const uint8_t * mac, const uint8_t *incomingData, int len) {
        memcpy(&packet, incomingData, sizeof(packet));

        if(packet>limInfVel && packet<limSupVel){
                if(!update_flag){
                        vel_current = packet;
                        update_flag = true;
                        Serial.println("hello world");
                        print_vel(vel_current);
                }
        }else{

        }
}

void setup() {
  
        btnConfig();
        
        tft.begin();
        
        tft.setRotation(4);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setCursor(10,10);
        tft.println("hello world");
        tft.setTextSize(4);

        delay(1000);


        espNow_begin();
        update_velocity();
        
        delay(1000);
        
        update_verification();

        unsigned long previousTime = millis();
        
        while(count_timeout < timeout){
                unsigned long currentTime = millis();
                
                if(conection_flag && update_flag){
                        btnHandler();
                }

                if(currentTime - previousTime >= 1000){
                        count_timeout++;

                        previousTime = currentTime;
                        update_verification();

                        if(!conection_flag){
                                espNow_begin();
                        }
                        if(!update_flag){
                                update_velocity();
                        }
                }
          
        }
        
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0); //1 = High, 0 = Low
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35,0); //1 = High, 0 = Low


        esp_deep_sleep_start();
}




void loop() {
  
  

}

void espNow_begin(){
        WiFi.mode(WIFI_STA);
        delay(100);

        if (esp_now_init() != ESP_OK) {
                return;
        }

        esp_now_register_send_cb(data_sent);
        
        
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;   
            
        if (esp_now_add_peer(&peerInfo) != ESP_OK){
                return;
        }else{
                conection_flag = true;
        }

        // Register for a callback function that will be called when data is received
        esp_now_register_recv_cb(data_receive);

}

void btnConfig(){
        btnL.setDebounceTime(50);
        btnR.setDebounceTime(150);
        
        pinMode(btn1, INPUT_PULLUP);
        attachInterrupt(btn1, isr_btn1, FALLING);
        
        pinMode(btn2, INPUT_PULLUP);
        attachInterrupt(btn2, isr_btn2, FALLING);
}

void btnHandler(){
  
  
        btnL.loop();
        btnR.loop();
        

        if (btnL.isPressed()) {
                vel_current -= vel_step;
               
                packet = vel_current;
                
                esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
                 
        }

        if (btnR.isPressed()) {
                vel_current += vel_step;

                packet = vel_current;

                esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
                 
        }

}


void isr_btn1(){
        count_timeout = 0;
  
}
void isr_btn2(){
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

void update_velocity(){
        request = 99999999;
        
        esp_now_send(broadcastAddress, (uint8_t *) &request, sizeof(packet));
}

void update_verification(){
        if(!update_flag){
                tft.fillRect(0,50,200,70,TFT_DARKGREY);
                tft.setTextColor(TFT_RED);
                tft.setCursor(30,70);
                tft.setTextSize(1);
                tft.print("No connection :(");
        }
}