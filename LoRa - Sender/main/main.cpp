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

#define xTaskSlave_FLAG (1<<0)
#define Interval 5000

TimerHandle_t xTimer;
TaskHandle_t xtaskHandle;
EventGroupHandle_t xEvents;

const String Get_Data = "getdata";
const String Set_Data = "setdata";
int count = 0;

// ----------------------------------------------------------------
// -----Task-----
void xTaskSlave (void *pvParameters);
void callBackTimer1(TimerHandle_t pxTimer);
String readData();

// ----------------------------------------------------------------
// -----Main Func-----
extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  SPI.begin();


  LoRa.setPins(LORA_CS, LORA_RST);

  pinMode(LORA_CS, OUTPUT);
  pinMode(LORA_RST, OUTPUT);

  if (!LoRa.begin(FREQ)){
    LoRa.begin(FREQ);
    Serial.println("Starting LoRa failed!");
  }

  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(SBW);
  LoRa.setTxPower(TX_POWER_DB); 
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  LoRa.setCodingRate4(CODING_RATE_DENOMINATOR);
 // LoRa.enableCrc();
  //LoRa.receive();
  Serial.println("LoRa Initizalized!");

  xEvents = xEventGroupCreate();
  xTimer = xTimerCreate("Timer", 
                        pdMS_TO_TICKS(1000), 
                        pdTRUE, 
                        0, 
                        callBackTimer1);

  xTaskCreate(xTaskSlave, 
              "Slave",
              configMINIMAL_STACK_SIZE + 1024,
              NULL,
              1,
              &xtaskHandle);

  xTimerStart(xTimer, 0);
}

void xTaskSlave(void *pvParameters){
  EventBits_t xBits;
  while(1){
    xBits = xEventGroupWaitBits(xEvents, xTaskSlave_FLAG, pdTRUE,pdTRUE, portMAX_DELAY);
    Serial.println("Enviando");
    vTaskDelay(1000);
  }
}

void callBackTimer1(TimerHandle_t pxTimer){
  int Packet_Size = LoRa.parsePacket();
  Serial.print(".");
  if (Packet_Size == Get_Data.length()){
    String received = "";

    while(LoRa.available()){
      received += (char)LoRa.read();
      Serial.println("preparando");
      
    }  

    if(strcmp(received.c_str(),  Get_Data.c_str()) == 0){
      Serial.println("enviando..");
      String data = "OPA";
      LoRa.beginPacket();
      LoRa.print(Set_Data);
      LoRa.println(data);
      LoRa.endPacket();
      Serial.print("Pacote enviado: ");
      Serial.println(data);
      xEventGroupSetBits(xEvents, xTaskSlave_FLAG);
    }
  }
}

String readData(){
  return String(count++);
}