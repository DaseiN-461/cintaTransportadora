/////////This is a remote device that control a variable veloctity by esp-now.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <esp_now.h>
 
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xCE, 0x50};
 

float temp;
float hum;

float sender_temperature;
float sender_humidity;

String success;

//Must match the receiver structure
typedef struct struct_message {
    float temp;
    float hum;
} struct_message;


struct_message DHT_Readings;
struct_message sender_Readings;
esp_now_peer_info_t peerInfo;
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
  memcpy(&sender_Readings, incomingData, sizeof(sender_Readings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  sender_temperature = sender_Readings.temp;
  sender_humidity = sender_Readings.hum;
}
 
void setup() {

  Serial.begin(115200);
  
/*
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
*/  
 
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
}
 
void loop() {
  
 
  DHT_Readings.temp = 20.2;
  DHT_Readings.hum = 12.3;
  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &DHT_Readings, sizeof(DHT_Readings));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
 
  delay(10000);
}

/*
void getReadings(){
  hum = dht.readHumidity();
 temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)  ){
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
}
}

*/


/*

void Display(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("INCOMING READINGS");
  display.setCursor(0, 15);
  display.print("Temperature: ");
  display.print(sender_temperature);
  display.cp437(true);
  display.write(248);
  display.print("C");
  display.setCursor(0, 25);
  display.print("Humidity: ");
  display.print(sender_humidity);
  display.print("%");
  display.setCursor(0, 56);
  display.print(success);
  display.display();
  
  Serial.println("INCOMING READINGS");
  Serial.print("Temperature: ");
  Serial.print(sender_Readings.temp);
  Serial.println(" ºC");
  Serial.print("Humidity: ");
  Serial.print(sender_Readings.hum);
  Serial.println(" %");
  Serial.println();
}

*/
