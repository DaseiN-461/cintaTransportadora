
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

int numberKeyPresses = 0;


int btn1 = 0;
int btn2 = 35;

void vAdd()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    numberKeyPresses++;
    Serial.print("#");
    Serial.print(numberKeyPresses);
    SerialBT.write(numberKeyPresses);
    Serial.println();
  }
  last_interrupt_time = interrupt_time;
}

void vDis()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    numberKeyPresses--;
    Serial.print("#");
    Serial.print(numberKeyPresses);
    SerialBT.write(numberKeyPresses);
    Serial.println();
  }
  last_interrupt_time = interrupt_time;
}

void setup() {
  pinMode(btn1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btn1), vAdd , FALLING);
  pinMode(btn2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btn2), vDis , FALLING);
 
  
  Serial.begin(115200);
  SerialBT.begin("Cinta transportadora"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}


void loop() {
  delay(100);
}
