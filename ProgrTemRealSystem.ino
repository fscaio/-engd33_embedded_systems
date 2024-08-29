#include <M5EPD.h>

M5EPD_Canvas realTime(&M5.EPD);
// M5EPD_Canvas canvas2(&M5.EPD);
// M5EPD_Canvas bateria(&M5.EPD);


#define FILE_NAME               "/PTRSE/DataLogger.txt"
#define MEMORYSTACKRTC          4096
#define MEMORYSTACKSPI          4096
#define MEMORYSTACKDATALOGGER   4096
#define MEMORYSTACKSimuEncoder  4096

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

void IHM(void);

void tarefaSPI(void * pvParameters);
void tarefaRTC(void * pvParameters);
void tarefaDataLogger(void * pvParameters);
void tarefaSBC(void * pvParameters);

void tarefaIHM(void * pvParameters);
void tarefaDadosSimuladorEncoder(void * pvParameters);

QueueHandle_t xQueueDatalog = NULL;
QueueHandle_t xQueueSPI = NULL;
QueueHandle_t xQueueCommSBC = NULL;
QueueHandle_t xQueueRTC = NULL;

QueueHandle_t xQueueSimuEncoder = NULL;

SemaphoreHandle_t xSemaphoreSPI = NULL;
SemaphoreHandle_t xSemaphoreRTC = NULL;
SemaphoreHandle_t xSemaphoreSBC = NULL;

SemaphoreHandle_t xMutex_var = NULL;


// rtc_time_t RTCtime;
//char timeStrbuff[64];

void setup() {

  M5.begin();
  M5.EPD.SetRotation(90);
  M5.TP.SetRotation(90);
   
  M5.EPD.Clear(true);
  realTime.createCanvas(480, 580);
  M5.RTC.begin();


  xMutex_var = xSemaphoreCreateMutex();

  xQueueDatalog = xQueueCreate(10, sizeof(DatalogEntry_t)); 
  xQueueSPI = xQueueCreate(10, sizeof(DatalogEntry_t));
  xQueueRTC = xQueueCreate(10, sizeof(char [64]));

  xQueueSimuEncoder = xQueueCreate(10, sizeof(uint8_t));
  //xQueueCommSBC = xQueueCreate(10, sizeof(uint8_t));

  xSemaphoreRTC = xSemaphoreCreateBinary();
  

  xTaskCreatePinnedToCore(&tarefaRTC, "RTCControl", 4096, NULL, 5, &taskRTC, tskNO_AFFINITY);   
  xTaskCreatePinnedToCore(&tarefaDataLogger, "DataLoggerControl", 4096, NULL, 4, &taskDataLogger, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSPI, "spiControl", 4096, NULL, 3, &taskSPI, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaIHM, "IHM", 4096, NULL, 2, &taskIHM, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaDadosSimuladorEncoder, "DadosSimuladorEncoder", 4096, NULL, 1, &taskSimuladorEncoder, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSBC, "SBCControl", 4096, NULL, 0, &taskSBC, tskNO_AFFINITY);

  vTaskStartScheduler();
}

void loop() {
  vTaskDelete(NULL);
}

void tarefaSPI(void * pvParameters){// SUSPEND 3 - IHM

  vTaskSuspend(NULL);

  Serial.println("Tarefa SPI Inicializada");
  Serial.print("tarefa SPI utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  File myFile;
  DatalogEntry_t SPIDdata;
  bool SDcard = false;

  if (!SD.begin()) {  // Inicialização do SD card.
      Serial.println("Falha no cartão, ou cartão não presente");  
  }else{
    Serial.println("Cartão de Memória encontrado");
    SDcard = true;
  }

  if(SDcard){
    if (SD.exists(FILE_NAME)) { 
                                    
      Serial.print(FILE_NAME);
      Serial.println(" foi encontrado.");

    } else {
      Serial.print(FILE_NAME);
      Serial.println(" não existe.");

      Serial.print("Criando o arquivo ");
      Serial.println(FILE_NAME);

      if(myFile = SD.open(FILE_NAME, FILE_WRITE)){
        Serial.println("Arquivo criado com sucesso");
        myFile.close();
      }else{
        Serial.println("Não foi possível criar o arquivo");
        SDcard = false;
      }
    }
  }

  vTaskResume(taskIHM);

  while(1){

    if(xQueueReceive(xQueueSPI, &SPIDdata, pdMS_TO_TICKS(100)) == pdPASS){
    
      if(SDcard){
        if(myFile = SD.open(FILE_NAME, FILE_APPEND)){

          myFile.print("Dados obtidos - ");
          myFile.print("Speed - ");
          myFile.print(SPIDdata.speed);
          myFile.print(" - Data - ");
          myFile.println(SPIDdata.horaDado);

          myFile.close();
        }else {
            Serial.print("Erro ao abrir o arquivo ");
            Serial.println(FILE_NAME);
        }
      }
    }

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    
    Serial.printf("\nTAREFA SPI OF %d - FREE MEMORY -> %u ", MEMORYSTACKSPI, uxHighWaterMark);
    //Serial.println(uxHighWaterMark);
    vTaskDelay(950/portTICK_PERIOD_MS);
  }
}

void tarefaRTC(void * pvParameters){ // 1 - datalogger

  //vTaskDelay(pdMS_TO_TICKS(50));
  Serial.println("Tarefa RTC Inicializada");
  Serial.print("Tarefa RTC utilizando o core -> ");
  Serial.println(xPortGetCoreID());
  
  rtc_time_t RTCtime;
  RTCtime.hour = 16;
  RTCtime.min  = 39;
  RTCtime.sec  = 00;
  char timeStrbuff[64];

  if (xSemaphoreTake(xMutex_var, portMAX_DELAY) == pdTRUE) {
  
    M5.RTC.setTime(&RTCtime);

    xSemaphoreGive(xMutex_var);
  }

  vTaskResume(taskDataLogger);

  while(1){

    
    M5.RTC.getTime(&RTCtime);
    snprintf(timeStrbuff, sizeof(timeStrbuff), "%02d:%02d:%02d", RTCtime.hour, RTCtime.min, RTCtime.sec);
    xQueueSend(xQueueRTC, &timeStrbuff, pdMS_TO_TICKS(100));
    Serial.print("\n - hora dentro da task rtc ");
    Serial.println(timeStrbuff);



    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("\nTAREFA RTC OF %d - FREE MEMORY -> %u ", MEMORYSTACKRTC, uxHighWaterMark);

    vTaskDelay(900/portTICK_PERIOD_MS);
  }
}


void tarefaDataLogger(void * pvParameters){//SUSPEND 2 - SPI
  vTaskSuspend(NULL);

  Serial.println("Tarefa DATALOGGER Inicializada");
  Serial.print("tarefa DataLogger utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  DatalogEntry_t Datalog;
  char timeStrbufff[64];
  uint8_t velEncoderDatalogger;

  vTaskResume(taskSPI);

  while(1){

    if(xQueueReceive(xQueueSimuEncoder, &velEncoderDatalogger, pdMS_TO_TICKS(100)) == pdPASS){
      //vTaskResume(taskRTC);
      Datalog.speed = velEncoderDatalogger;
      if(xQueueReceive(xQueueRTC, &timeStrbufff, pdMS_TO_TICKS(100)) == pdPASS){
        strncpy(Datalog.horaDado, timeStrbufff, sizeof(timeStrbufff));
      }
      Serial.print("\n speed - ");
      Serial.println(Datalog.speed);
      Serial.print("\n hora - ");
      Serial.println(Datalog.horaDado);


      xQueueSend(xQueueSPI,&Datalog, pdMS_TO_TICKS(100));


    }
    //if(xQueueReceive(xQueueRTC, &Datalog, pdMS_TO_TICKS(100)) == pdPASS){}

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("\nTAREFA DATALOGGER OF %d - FREE MEMORY -> %u ", MEMORYSTACKDATALOGGER, uxHighWaterMark);
    //Serial.println(uxHighWaterMark);

    vTaskDelay(1100/portTICK_PERIOD_MS);
  }
}

void tarefaSBC(void * pvParameters){//SUSPEND
  vTaskSuspend(NULL);
  while(1){
    
  }
}

void tarefaIHM(void * pvParameters){ //SUSPEND 4 - SIMULADOR
  vTaskSuspend(NULL);

  Serial.println("Tarefa IHM Inicializada");
  Serial.print("tarefa IHM utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  if (xSemaphoreTake(xMutex_var, portMAX_DELAY) == pdTRUE) {
  
    realTime.setTextSize(3);
    realTime.setTextArea(200,200,100,100);

    xSemaphoreGive(xMutex_var);
  }

  vTaskResume(taskSimuladorEncoder);

  while(1){
    
    if (xSemaphoreTake(xMutex_var, portMAX_DELAY) == pdTRUE) {
      M5.update();
      xSemaphoreGive(xMutex_var);
    }
    
    realTime.print("ola");
    realTime.drawNumber(xPortGetCoreID(), 20, 20);
    realTime.pushCanvas(0, 0, UPDATE_MODE_DU4);

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("\n quantidade de memoria em palavra restante do stack - TAREFA IHM -> ");
    Serial.println(uxHighWaterMark);

    vTaskDelay(1300/portTICK_PERIOD_MS);
  }
}

void tarefaDadosSimuladorEncoder(void * pvParameters){//SUSPEND 5 - 
  vTaskSuspend(NULL);
  
  Serial.println("Tarefa IHM Inicializada");
  Serial.print("tarefa SimuladorEconder utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  uint8_t velEncoder = 0;

  while(1){

    velEncoder = velEncoder + 10;
    if(velEncoder > 240)
      velEncoder = 0;

    xQueueSend(xQueueSimuEncoder, &velEncoder, pdMS_TO_TICKS(100));

    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

