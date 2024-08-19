
#deine FILE_NAME        "/PTRSE/DataLogger.txt"



TaskHandle_t taskSPI = NULL;
TaskHandle_t taskRTC = NULL;
TaskHandle_t taskDataLogger = NULL;
TaskHandle_t taskSBC = NULL;

void tarefaSPI(void * pvParameters);
void tarefaRTC(void * pvParameters);
void tarefaDataLogger(void * pvParameters);
void tarefaSBC(void * pvParameters);

QueueHandle_t xQueueDatalog;
QueueHandle_t xQueueSPI;
QueueHandle_t xQueueCommSBC;



void setup() {

  

  xTaskCreatePinnedToCore(&tarefaSPI, "spiControl", 4096, NULL, 0, &taskSPI, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaRTC, "RTCControl", 4096, NULL, 0, &taskRTC, tskNO_AFFINITY);
  TaskCreatePinnedToCore(&tarefaDataLogger, "spiControl", 4096, NULL, 0, &tarefaDataLogger, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(&tarefaSBC, "RTCControl", 4096, NULL, 0, &taskSBC, tskNO_AFFINITY);
  vTaskStartScheduler();
}

void loop() {
  vTaskDelete(NULL);

}

void tarefaSPI(void * pvParameters){

  while(1){

  }
}

void tarefaRTC(void * pvParameters){

  while(1){
    
  }
}

void tarefaDataLogger(void * pvParameters){

  while(1){

  }
}

void tarefaSBC(void * pvParameters){

  while(1){
    
  }
}
