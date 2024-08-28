
#include <M5EPD.h>

M5EPD_Canvas realTime(&M5.EPD);
M5EPD_Canvas canvas2(&M5.EPD);
M5EPD_Canvas bateria(&M5.EPD);


#define FILE_NAME        "/PTRSE/DataLogger.txt"

typedef struct {
    char horaDado[64];
    uint8_t speed;
} DatalogEntry_t;

typedef struct{
    uint32_t timestamp;
    float sensorValue;
    float speed;
} SPIPacket_t;


TaskHandle_t taskSPI = NULL;
TaskHandle_t taskRTC = NULL;
TaskHandle_t taskDataLogger = NULL;
TaskHandle_t taskSBC = NULL;

TaskHandle_t taskSimuladorEncoder = NULL;
TaskHandle_t taskIHM = NULL;

void tarefaSPI(void * pvParameters);
void tarefaRTC(void * pvParameters);
void tarefaDataLogger(void * pvParameters);
void tarefaSBC(void * pvParameters);

void tarefaIHM(void * pvParameters);
void tarefaDadosSimuladorEncoder(void * pvParameters);

QueueHandle_t xQueueDatalog;
QueueHandle_t xQueueSPI;
QueueHandle_t xQueueCommSBC;
QueueHandle_t xQueueRTC;

QueueHandle_t xQueueSimuEncoder;

SemaphoreHandle_t xSemaphoreSPI;
SemaphoreHandle_t xSemaphoreRTC;
SemaphoreHandle_t xSemaphoreSBC;

// rtc_time_t RTCtime;
char timeStrbuff[64];

void setup() {

   M5.begin();
   M5.EPD.SetRotation(90);
   M5.TP.SetRotation(90);
   //M5.RTC.begin();
   M5.EPD.Clear(true);

  // if (!SD.begin()) {  // Inicialização do SD card.
  //     Serial.println("Card failed, or not present");  
  // }


  xQueueDatalog = xQueueCreate(10, sizeof(DatalogEntry_t)); 
  xQueueSPI = xQueueCreate(10, sizeof(DatalogEntry_t));
  xQueueRTC = xQueueCreate(10, sizeof(char [64]));

  xQueueSimuEncoder = xQueueCreate(10, sizeof(uint8_t));
  //xQueueCommSBC = xQueueCreate(10, sizeof(uint8_t));

  xSemaphoreRTC = xSemaphoreCreateBinary();
  

  xTaskCreatePinnedToCore(&tarefaSPI, "spiControl", 4096, NULL, 0, &taskSPI, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaRTC, "RTCControl", 4096, NULL, 0, &taskRTC, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaDataLogger, "DataLoggerControl", 4096, NULL, 0, &taskDataLogger, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSBC, "RTCControl", 4096, NULL, 0, &taskSBC, tskNO_AFFINITY);

  xTaskCreatePinnedToCore(&tarefaIHM, "IHM", 4096, NULL, 0, &taskIHM, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaDadosSimuladorEncoder, "DadosSimuladorEncoder", 1024, NULL, 1, &taskSimuladorEncoder, tskNO_AFFINITY);

  vTaskStartScheduler();
}

void loop() {
  vTaskDelete(NULL);
}

void tarefaSPI(void * pvParameters){

  File myFile;
  SPIPacket_t SPIDdata;
  bool SDcard = false;

  if (!SD.begin()) {  // Inicialização do SD card.
      Serial.println("Falha no cartão, ou cartão não presente");  
  }else{
    Serial.println("Cartão de Memória encontrado");
    SDcard = true;
  }

  if(SDcard){
    if (SD.exists("/RealTime.txt")) { 
                                    
      Serial.println("RealTime.txt foi encontrado.");

    } else {

      Serial.println("RealTime.txt não existe.");

      Serial.println("Criando o arquivo RealTime.txt");

      if(myFile = SD.open("/RealTime.txt", FILE_WRITE)){
        Serial.println("Arquivo criado com sucesso");
        myFile.close();
      }else{
        Serial.println("Não foi possível criar o arquivo");
        SDcard = false;
      }
    }
  }

  while(1){
    if(xQueueReceive(xQueueSPI, &SPIDdata, pdMS_TO_TICKS(100)) == pdPASS){}
    
    if(SDcard){
      if(myFile = SD.open("/RealTime.txt", FILE_WRITE)){


      }else {
          Serial.println("Erro ao abrir o arquivo RealTime.txt");
      }
    }
  }
}

void tarefaRTC(void * pvParameters){
  M5.RTC.begin();
  
  rtc_time_t RTCtime;
  RTCtime.hour = 16;
  RTCtime.min  = 39;
  RTCtime.sec  = 00;
  //char timeStrbuff[64];
  M5.RTC.setTime(&RTCtime);

  while(1){

    //xSemaphoreTake(xSemaphoreRTC, portMAX_DELAY);
      M5.RTC.getTime(&RTCtime);
      sprintf(timeStrbuff, "%02d:%02d:%02d", RTCtime.hour, RTCtime.min, RTCtime.sec);
      Serial.print("\n - hora dentro da task rtc ");
      Serial.println(timeStrbuff);
    
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
}


void tarefaDataLogger(void * pvParameters){
  DatalogEntry_t Datalog;
  char timeStrbuff[64];
  uint8_t velEncoderDatalogger;

  while(1){

    if(xQueueReceive(xQueueSimuEncoder, &velEncoderDatalogger, pdMS_TO_TICKS(100)) == pdPASS){
      Datalog.speed = velEncoderDatalogger;
      strcpy(Datalog.horaDado, timeStrbuff);
      Serial.print("\n speed - ");
      Serial.println(Datalog.speed);
      Serial.print("\n hora - ");
      Serial.println(timeStrbuff);


      xQueueSend(xQueueSPI,&Datalog, portMAX_DELAY);


    }
    if(xQueueReceive(xQueueRTC, &Datalog, pdMS_TO_TICKS(100)) == pdPASS){}

    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void tarefaSBC(void * pvParameters){
  vTaskSuspend(NULL);
  while(1){
    
  }
}

void tarefaIHM(void * pvParameters){
  vTaskSuspend(NULL);
  while(1){
    
  }
}

void tarefaDadosSimuladorEncoder(void * pvParameters){
  //vTaskSuspend(NULL);
  uint8_t velEncoder = 0;

  while(1){

    velEncoder = velEncoder + 10;
    if(velEncoder > 240)
      velEncoder = 0;

    xQueueSend(xQueueSimuEncoder, &velEncoder, pdMS_TO_TICKS(100));

    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}
