//#include <Arduino.h>


#include <esp_now.h>
 
#include <WiFi.h>

#include <ezButton.h>

#include <TFT_eSPI.h>
#include <SPI.h>




uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xC1, 0xFC, 0x30};
 
bool update_flag = false;

String success;

int packet;



esp_now_peer_info_t peerInfo;


int btn1 = 0;
int btn2 = 35;


int numberKeyPresses = 0;
int vel_step = 5;

int vel_VFD;
int vel_current;

int count_timeout = 0;
int timeout = 10;

////////////////////////////////////////////////Defines for DeepSleep//////////////////////////////////////////////

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

RTC_DATA_ATTR int bootCount = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TFT_eSPI tft = TFT_eSPI();

ezButton btnL(btn1);
ezButton btnR(btn2);


void tft_init();
void print_vel(int vel);

void btnConfig();
void btnHandler();
void print_wakeup_reason();
void isr_btn1();
void isr_btn2();


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
  Serial.print("Packet received: ");
  Serial.println(packet);
  if(!update_flag){
    vel_current = packet;
    update_flag = true;
    Serial.println("hello world");
    print_vel(vel_current);
  }
  
  

  
}


void setup() {
  
  btnConfig();
  
  Serial.begin(115200);
  

  tft.begin();
  
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  tft.setCursor(10,10);
  tft.println("hello world");

  tft.setTextSize(4);

  
  
  delay(1000);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////   Debugging                    /////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////      Pair with BLE Server      /////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  WiFi.mode(WIFI_STA);
  delay(100);
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

  
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////     Read velocity from VFD     /////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  int vel_request = 99;
  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &vel_request, sizeof(packet));
     
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////     Loop, process and transmit vel     //////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    

    
  unsigned long previousTime = millis();
  
  while(count_timeout < timeout){
    unsigned long currentTime = millis();

    btnHandler();
     
    if(currentTime - previousTime >= 1000){
      count_timeout++;
      Serial.printf("time out: [%d]\r\n", count_timeout);

      previousTime = currentTime;
    }
    
  }
                   ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////          if timeout            /////////////////////////////////////////
                   ///////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////     Send data to BLE Server
                   ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////        Go to Deep Sleep        /////////////////////////////////////////
                   ///////////////////////////////////////////////////////////////////////////////////////////

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0); //1 = High, 0 = Low
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35,0); //1 = High, 0 = Low

  //If you were to use ext1, you would use it like
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}




void loop() {
  
  

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
    //numberKeyPresses--;
    vel_current -= vel_step;
    Serial.printf("velocity: [%d]\r\n",vel_current);
    print_vel(vel_current);
    //Serial.println(numberKeyPresses);
    packet = vel_current;
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
     
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    
  }

  if (btnR.isPressed()) {
    //numberKeyPresses++;
    vel_current += vel_step;
    Serial.printf("velocity: [%d]\r\n",vel_current);
    print_vel(vel_current);
    //Serial.println(numberKeyPresses);

    packet = vel_current;
    Serial.print("Packet to send: ");
    Serial.println(packet);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
     
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
/* 
  DHT_Readings.temp = vel_step;
  DHT_Readings.hum = 99;
  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &DHT_Readings, sizeof(DHT_Readings));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
*/
}


void isr_btn1(){
  count_timeout = 0;
  //btnHandler();  
  
  
}
void isr_btn2(){
  count_timeout = 0;
  //btnHandler();
  
  
}


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void tft_init(){
  tft.begin();
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
}

void print_vel(int vel){
    tft.fillRect(10,50,100,50,TFT_DARKGREY);
    tft.setTextColor(TFT_RED);
    tft.setCursor(30,70);
    tft.print(vel);
}
