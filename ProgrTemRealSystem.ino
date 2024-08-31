#include <M5EPD.h>

M5EPD_Canvas realTime(&M5.EPD);
M5EPD_Canvas taskInfo(&M5.EPD);
// M5EPD_Canvas bateria(&M5.EPD);


#define FILE_NAME               "/DataLogger.txt"

#define MEMORYSTACKRTC          1800
#define MEMORYSTACKSPI          2800
#define MEMORYSTACKDATALOGGER   2200
#define MEMORYSTACKSimuEncoder  1700
#define MEMORYSTACKSimuAccGyro  1700
#define MEMORYSTACKIHM          1500


typedef struct {
    char horaDado[64];
    uint32_t data;
} DatalogEntryEncoder_t;

typedef struct{
  int AcX;
  int AcY;
  int AcZ;
  int GyX;
  int GyY;
  int GyZ;

} AccGyro;

typedef struct{
    char horaDado[64];
    AccGyro AceLGyro;
} DatalogEntryAccGyro_t;

struct IHMDADOS{
  uint8_t tarefa;
  UBaseType_t memoria;
  BaseType_t core;
};


bool SDcard = false;


TaskHandle_t taskSPI = NULL;
TaskHandle_t taskRTC = NULL;
TaskHandle_t taskDataLogger = NULL;
TaskHandle_t taskSBC = NULL;

TaskHandle_t taskSimuladorEncoder = NULL;
TaskHandle_t taskSimuladorAccGyro = NULL;
TaskHandle_t taskIHM = NULL;

void IHM(void);

void tarefaSPI(void * pvParameters);
void tarefaRTC(void * pvParameters);
void tarefaDataLogger(void * pvParameters);
void tarefaSBC(void * pvParameters);

void tarefaIHM(void * pvParameters);
void tarefaDadosSimuladorEncoder(void * pvParameters);
void tarefaDadosSimuladorAccGyro(void * pvParameters);

QueueHandle_t xQueueDatalog = NULL;
QueueHandle_t xQueueSPIEncoder = NULL; //QUEUE QUE ENVIA PARA O SPI
QueueHandle_t xQueueSPIAccGyro = NULL;

QueueHandle_t xQueueRTC = NULL; //QUEUE RTC QUE ENVIA PARA O DATALOGGER

QueueHandle_t xQueueSimuEncoder = NULL; //QUEUE ENCONDER QUE ENVIA DADOS PARA DATALOGGUER
QueueHandle_t xQueueSimuAccGyro = NULL; //QUEUE ACCGYRO QUE ENVIA DADOS PARA O DATALOGGER

QueueHandle_t xQueueIHMDADOS = NULL;


//QueueHandle_t xQueueCommSBC = NULL;

SemaphoreHandle_t xSemaphoreSPI = NULL;
SemaphoreHandle_t xSemaphoreRTC = NULL;
SemaphoreHandle_t xSemaphoreSBC = NULL;

SemaphoreHandle_t xMutex_var = NULL;


// rtc_time_t RTCtime;
//char timeStrbuff[64];

void setup() {

  M5.begin();
  M5.EPD.SetRotation(0);
  M5.TP.SetRotation(0);
   
  M5.EPD.Clear(true);
  delay(100);
  //realTime.createCanvas(960, 540);
  taskInfo.createCanvas(960, 540);
  M5.RTC.begin();
  delay(100);

  if (!SD.begin()) {  // Inicialização do SD card.
    Serial.println("Falha no cartão, ou cartão não presente");  
  }else{
    Serial.println("Cartão de Memória encontrado");
    SDcard = true;
  }
  delay(100);

  randomSeed(55);


  xMutex_var = xSemaphoreCreateMutex();

  xQueueDatalog = xQueueCreate(10, sizeof(DatalogEntryEncoder_t));

  xQueueSPIEncoder = xQueueCreate(10, sizeof(DatalogEntryEncoder_t));
  xQueueSPIAccGyro = xQueueCreate(10, sizeof(DatalogEntryAccGyro_t));
  xQueueRTC = xQueueCreate(10, sizeof(char [64]));

  xQueueSimuEncoder = xQueueCreate(10, sizeof(uint32_t));
  xQueueSimuAccGyro = xQueueCreate(10, sizeof(AccGyro));

  xQueueIHMDADOS = xQueueCreate(50, sizeof(IHMDADOS));
  //xQueueCommSBC = xQueueCreate(10, sizeof(uint8_t));

  xSemaphoreRTC = xSemaphoreCreateBinary();
  

  xTaskCreatePinnedToCore(&tarefaRTC, "RTCControl", MEMORYSTACKRTC, NULL, 5, &taskRTC, tskNO_AFFINITY);   
  xTaskCreatePinnedToCore(&tarefaDataLogger, "DataLoggerControl", MEMORYSTACKDATALOGGER, NULL, 4, &taskDataLogger, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSPI, "spiControl", MEMORYSTACKSPI, NULL, 3, &taskSPI, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaIHM, "IHM", MEMORYSTACKIHM, NULL, 2, &taskIHM, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaDadosSimuladorEncoder, "DadosSimuladorEncoder", MEMORYSTACKSimuEncoder, NULL, 1, &taskSimuladorEncoder, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaDadosSimuladorAccGyro, "DadosSimuladorAccGyro", MEMORYSTACKSimuAccGyro, NULL, 1, &taskSimuladorAccGyro, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSBC, "SBCControl", 4096, NULL, 0, &taskSBC, tskNO_AFFINITY);

  //vTaskStartScheduler();
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
  DatalogEntryEncoder_t SPIDataEncoder;
  DatalogEntryAccGyro_t SPIDataAccGyro;
  IHMDADOS RTCtoIHM;
  

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

    if(xQueueReceive(xQueueSPIEncoder, &SPIDataEncoder, pdMS_TO_TICKS(100)) == pdPASS){
    
      if(SDcard){
        if(myFile = SD.open(FILE_NAME, FILE_APPEND)){

          myFile.print("Dados obtidos - ");
          myFile.print("Speed - ");
          myFile.print(SPIDataEncoder.data);
          myFile.print(" - Data - ");
          myFile.println(SPIDataEncoder.horaDado);

          myFile.close();
          Serial.println("\n Dado gravado com suceso");
        }else {
            Serial.print("Erro ao abrir o arquivo ");
            Serial.println(FILE_NAME);
        }
      }
    }
    if(xQueueReceive(xQueueSPIAccGyro, &SPIDataAccGyro, pdMS_TO_TICKS(100)) == pdPASS){
    
      if(SDcard){
        if(myFile = SD.open(FILE_NAME, FILE_APPEND)){

          myFile.print("Dados obtidos - ");
          myFile.printf("Acelerômetro - %d x - %d y - %d z -> Gyroscopio - %d x - %d y - %d z", SPIDataAccGyro.AceLGyro.AcX,
                                                                                                SPIDataAccGyro.AceLGyro.AcY,
                                                                                                SPIDataAccGyro.AceLGyro.AcZ,
                                                                                                SPIDataAccGyro.AceLGyro.GyX,
                                                                                                SPIDataAccGyro.AceLGyro.GyY,
                                                                                                SPIDataAccGyro.AceLGyro.GyZ);
          
          myFile.print(" - Data - ");
          myFile.println(SPIDataAccGyro.horaDado);

          myFile.close();
        }else {
            Serial.print("Erro ao abrir o arquivo ");
            Serial.println(FILE_NAME);
        }
      }
    }

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    
    Serial.printf("\nTAREFA SPI OF %d - FREE MEMORY -> %u ", MEMORYSTACKSPI, uxHighWaterMark);

    RTCtoIHM.tarefa = 2;
    RTCtoIHM.memoria = uxHighWaterMark;
    RTCtoIHM.core = xPortGetCoreID();
    xQueueSend(xQueueIHMDADOS, &RTCtoIHM, pdMS_TO_TICKS(100));
    //Serial.println(uxHighWaterMark);
    vTaskDelay(950/portTICK_PERIOD_MS);
  }
}
//----------------FUNÇÃO AUXILIAR DO SPI--------------------
//void SDcard(void){

//}

void tarefaRTC(void * pvParameters){ // 1 - datalogger

  Serial.println("Tarefa RTC Inicializada");
  //Serial.flush();
  Serial.print("Tarefa RTC utilizando o core -> ");
  Serial.println(xPortGetCoreID());
  
  rtc_time_t RTCtime;
  RTCtime.hour = 16;
  RTCtime.min  = 39;
  RTCtime.sec  = 00;
  char timeStrbuff[64];
  IHMDADOS RTCtoIHM;

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
    RTCtoIHM.tarefa = 1;
    RTCtoIHM.memoria = uxHighWaterMark;
    RTCtoIHM.core = xPortGetCoreID();
    xQueueSend(xQueueIHMDADOS, &RTCtoIHM, pdMS_TO_TICKS(100));

    vTaskDelay(900/portTICK_PERIOD_MS);
  }
}

void tarefaDataLogger(void * pvParameters){//SUSPEND 2 - SPI
  vTaskSuspend(NULL);

  Serial.println("Tarefa DATALOGGER Inicializada");
  Serial.print("tarefa DataLogger utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  DatalogEntryEncoder_t DatalogEncoderTime;
  DatalogEntryAccGyro_t DataAccGyroTime;
  AccGyro DataAccGyro;
  char timeStrbufff[64];
  uint32_t velEncoderDatalogger;
  IHMDADOS RTCtoIHM;

  vTaskResume(taskSPI);

  while(1){

    if(xQueueReceive(xQueueSimuEncoder, &velEncoderDatalogger, pdMS_TO_TICKS(100)) == pdPASS){
      //vTaskResume(taskRTC);
      DatalogEncoderTime.data = velEncoderDatalogger;
      if(xQueueReceive(xQueueRTC, &timeStrbufff, pdMS_TO_TICKS(100)) == pdPASS){
        strncpy(DatalogEncoderTime.horaDado, timeStrbufff, sizeof(timeStrbufff));
      }
      Serial.print("\n speed - ");
      Serial.print(DatalogEncoderTime.data);
      Serial.print(" - hora - ");
      Serial.println(DatalogEncoderTime.horaDado);


      xQueueSend(xQueueSPIEncoder,&DatalogEncoderTime, pdMS_TO_TICKS(100));

    }
    if(xQueueReceive(xQueueSimuAccGyro, &DataAccGyro, pdMS_TO_TICKS(100)) == pdPASS){
      DataAccGyroTime.AceLGyro.AcX = DataAccGyro.AcX;
      DataAccGyroTime.AceLGyro.AcY = DataAccGyro.AcY;
      DataAccGyroTime.AceLGyro.AcZ = DataAccGyro.AcZ;
      DataAccGyroTime.AceLGyro.GyX = DataAccGyro.GyX;
      DataAccGyroTime.AceLGyro.GyY = DataAccGyro.GyY;
      DataAccGyroTime.AceLGyro.GyZ = DataAccGyro.GyZ;
      
      if(xQueueReceive(xQueueRTC, &timeStrbufff, pdMS_TO_TICKS(100)) == pdPASS){
        strncpy(DataAccGyroTime.horaDado, timeStrbufff, sizeof(timeStrbufff));
      }
      Serial.print("\nDados obtidos - ");
      Serial.printf("Acelerômetro - %d x - %d y - %d z -> Gyroscopio - %d x - %d y - %d z", DataAccGyroTime.AceLGyro.AcX,
                                                                                            DataAccGyroTime.AceLGyro.AcY,
                                                                                            DataAccGyroTime.AceLGyro.AcZ,
                                                                                            DataAccGyroTime.AceLGyro.GyX,
                                                                                            DataAccGyroTime.AceLGyro.GyY,
                                                                                            DataAccGyroTime.AceLGyro.GyZ);
      
      Serial.print(" - Data - ");
      Serial.println(DataAccGyroTime.horaDado);

      xQueueSend(xQueueSPIAccGyro,&DataAccGyroTime, pdMS_TO_TICKS(100));
    }

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("\nTAREFA DATALOGGER OF %d - FREE MEMORY -> %u ", MEMORYSTACKDATALOGGER, uxHighWaterMark);
    RTCtoIHM.tarefa = 3;
    RTCtoIHM.memoria = uxHighWaterMark;
    RTCtoIHM.core = xPortGetCoreID();
    xQueueSend(xQueueIHMDADOS, &RTCtoIHM, pdMS_TO_TICKS(100));
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

  IHMDADOS IHMDADOSRECEBIDOS;
  UBaseType_t uxHighWaterMark;
  tp_finger_t dadosTouch;
  int8_t pagina = 0;



  if (xSemaphoreTake(xMutex_var, portMAX_DELAY) == pdTRUE) {
  
    //realTime.setTextSize(3);
    taskInfo.setTextSize(3);
    //realTime.setTextArea(200,200,100,100);

    xSemaphoreGive(xMutex_var);
  }

  vTaskResume(taskSimuladorEncoder);
  vTaskResume(taskSimuladorAccGyro);

  while(1){
    
    if (xSemaphoreTake(xMutex_var, portMAX_DELAY) == pdTRUE) {
      M5.update();
      xSemaphoreGive(xMutex_var);
    }

    // if(M5.TP.available()){
    //   if(!M5.TP.isFingerUp()){
    //     //M5.TP.update();
    //     dadosTouch = M5.TP.readFinger(1);

    //     if(dadosTouch.x >= 480){
    //       pagina++;
    //       if(pagina > 1);
    //         pagina = 1;
    //     }else if (dadosTouch.x < 480) {
    //       pagina--;
    //       if(pagina < 0);
    //         pagina = 0;
    //     }
    //   }
    // }
    // taskInfo.drawNumber(pagina, 480,140);
    // taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);

    if(xQueueReceive(xQueueIHMDADOS, &IHMDADOSRECEBIDOS, portMAX_DELAY) == pdPASS){
      switch(IHMDADOSRECEBIDOS.tarefa){
        case 1:
          taskInfo.drawString("Tarefa RTC", 30,100);
          taskInfo.drawString("Memory FREE", 40,140);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.memoria, 270,140);
          taskInfo.drawString("Core ->", 40,180);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.core, 200,180);
          taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);

          break;
        case 2:
          taskInfo.drawString("Tarefa SPI", 30,220);
          taskInfo.drawString("Memory FREE", 40,260);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.memoria, 270,260);
          taskInfo.drawString("Core ->", 40,300);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.core, 200,300);
          taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);

          break;
        case 3:
          taskInfo.drawString("Tarefa DATALOGGER", 30,340);
          taskInfo.drawString("Memory FREE", 40,380);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.memoria, 270,380);
          taskInfo.drawString("Core ->", 40,420);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.core, 200,420);
          taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);
          break;

        case 4:
          taskInfo.drawString("Tarefa SIMUACCGYRO", 510,100);
          taskInfo.drawString("Memory FREE", 520,140);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.memoria, 750, 140);
          taskInfo.drawString("Core ->", 520,180);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.core, 680,180);
          taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);
          break;

        case 5:
          taskInfo.drawString("Tarefa SIMUENCODER", 510,220);
          taskInfo.drawString("Memory FREE", 520,260);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.memoria, 750, 260);
          taskInfo.drawString("Core ->", 520,300);
          taskInfo.drawNumber(IHMDADOSRECEBIDOS.core, 680,300);
          taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);
          break;
      }
      taskInfo.drawString("Tarefa IHM", 510,340);
      taskInfo.drawString("Memory FREE", 520,380);
      taskInfo.drawNumber(uxHighWaterMark, 750, 380);
      taskInfo.drawString("Core ->", 520,420);
      taskInfo.drawNumber(xPortGetCoreID(), 680,420);
      taskInfo.pushCanvas(0, 0, UPDATE_MODE_DU4);
    }
    
    // realTime.print("ola");
    // realTime.drawNumber(xPortGetCoreID(), 20, 20);
    // realTime.pushCanvas(0, 0, UPDATE_MODE_DU4);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("\n quantidade de memoria em palavra restante do stack - TAREFA IHM -> ");
    Serial.println(uxHighWaterMark);

    vTaskDelay(1300/portTICK_PERIOD_MS);
  }
}

void tarefaDadosSimuladorEncoder(void * pvParameters){//SUSPEND 5 - 
  vTaskSuspend(NULL);
  
  Serial.println("Tarefa SimuEncoder Inicializada");
  Serial.print("tarefa SimuladorEconder utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  uint32_t velEncoder = 0;
  IHMDADOS RTCtoIHM;

  while(1){

    velEncoder = velEncoder + 10;
    if(velEncoder > 240)
      velEncoder = 0;

    xQueueSend(xQueueSimuEncoder, &velEncoder, pdMS_TO_TICKS(100));

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("\nTAREFA SIMUACCGYR OF %d - FREE MEMORY -> %u ", MEMORYSTACKSimuEncoder, uxHighWaterMark);
    RTCtoIHM.tarefa = 5;
    RTCtoIHM.memoria = uxHighWaterMark;
    RTCtoIHM.core = xPortGetCoreID();
    xQueueSend(xQueueIHMDADOS, &RTCtoIHM, pdMS_TO_TICKS(10));

    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void tarefaDadosSimuladorAccGyro(void * pvParameters){//SUSPEND 5 - 
  vTaskSuspend(NULL);
  
  Serial.println("Tarefa SimuAccGyro Inicializada");
  Serial.print("tarefa SimuladorEconder utilizando o core -> ");
  Serial.println(xPortGetCoreID());

  AccGyro AcelerometroGyroscopio;
  IHMDADOS RTCtoIHM;

  while(1){

    AcelerometroGyroscopio.AcX = random(-100, 100);
    AcelerometroGyroscopio.AcY = random(-100, 100);
    AcelerometroGyroscopio.AcZ = random(-100, 100);
    AcelerometroGyroscopio.GyX = random(-100, 100);
    AcelerometroGyroscopio.GyY = random(-100, 100);
    AcelerometroGyroscopio.GyZ = random(-100, 100);

    xQueueSend(xQueueSimuAccGyro, &AcelerometroGyroscopio, pdMS_TO_TICKS(100));

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("\nTAREFA SIMUACCGYR OF %d - FREE MEMORY -> %u ", MEMORYSTACKSimuAccGyro, uxHighWaterMark);
    RTCtoIHM.tarefa = 4;
    RTCtoIHM.memoria = uxHighWaterMark;
    RTCtoIHM.core = xPortGetCoreID();
    xQueueSend(xQueueIHMDADOS, &RTCtoIHM, pdMS_TO_TICKS(10));

    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

