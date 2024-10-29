//#include "PubSubClient.h"
#include "SPI.h"
#include "Arduino.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "stdio.h"
#include "LoRa.h"
#include "rtc_wdt.h"

// ---------------------------------------------------------------- 
// ---defines---
#define   SF                      9
#define   FREQ                    915E6
#define   SBW                     62.5E3	
#define   SYNC_WORD               0x34
#define   TX_POWER_DB             17
#define   PREAMBLE_LENGTH         8
#define   CODING_RATE_DENOMINATOR 5

// ----------------------------------------------------------------  
//----pins------
#define   LORA_CS       21
#define   LORA_RST      22

#define xTaskReceiver_FLAG (1<<0)
#define Interval 500

TimerHandle_t xTimer;
TaskHandle_t XtaskHandle;
EventGroupHandle_t xEvents;

long LastSendTime = 0;

void  receive();
void  Send();

const String Get_Data = "getdata";
const String Set_Data = "setdata";


// ----------------------------------------------------------------
// -----Task-----
void xTaskReceiver (void *pvParameters);
void callBackTimer1(TimerHandle_t pxTimer);

// ----------------------------------------------------------------
// -----Main Func-----
extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  SPI.begin();

  LoRa.setPins(LORA_CS, LORA_RST);

  pinMode(LORA_CS, OUTPUT);
  pinMode(LORA_RST, OUTPUT);

  while (!LoRa.begin(FREQ)){
    LoRa.begin(FREQ);
    Serial.println("Starting LoRa failed!");
  }

  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(SBW);
  LoRa.setTxPower(TX_POWER_DB); 
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  LoRa.setCodingRate4(CODING_RATE_DENOMINATOR);

  Serial.println("LoRa Initizalized!");

  xEvents = xEventGroupCreate();
  xTimer = xTimerCreate("Timer", 
                        pdMS_TO_TICKS(1000), 
                        pdTRUE, 
                        0, 
                        callBackTimer1);


  xTaskCreate(xTaskReceiver, 
              "receiver",
              configMINIMAL_STACK_SIZE + 1024,
              NULL,
              1,
              &XtaskHandle);

  xTimerStart(xTimer, 0);
}

void xTaskReceiver(void *pvParameters){
  EventBits_t xBits;
  while(1){
    xBits = xEventGroupWaitBits(xEvents, xTaskReceiver_FLAG, pdTRUE,pdTRUE, portMAX_DELAY);
    //Serial.println("Solicitando");
    vTaskDelay(1000);
  }
}

void callBackTimer1(TimerHandle_t pxTimer){
  if (millis() - LastSendTime > Interval){
    xEventGroupSetBits(xEvents, xTaskReceiver_FLAG);
    LastSendTime = millis();
    Send();
  }
  receive(); 
}

void  Send(){
  LoRa.beginPacket();
  LoRa.print(Get_Data);
  LoRa.endPacket();
}

void  receive(){
  int packet_Size  = LoRa.parsePacket();
  Serial.print("packet: ");
  Serial.println(packet_Size);
  Serial.print("data: ");
  Serial.println(Set_Data.length());
  if (packet_Size > Set_Data.length()){
    String Received = "";
    while (LoRa.available()){
      Serial.println("Glockada de 30");
      Received  += (char)LoRa.read();
    }
    int index = Received.indexOf(Set_Data);
    if  (index >= 0){
      Serial.print("Pididieeee");
      String data = Received.substring(Set_Data.length());
      String waiting = String(millis() -  LastSendTime);
      
      Serial.print("Recebido: ");
      Serial.println(data);
      Serial.print("Tempo de espera: ");
      Serial.print(waiting);
      Serial.println("ms");
    }
  }
}