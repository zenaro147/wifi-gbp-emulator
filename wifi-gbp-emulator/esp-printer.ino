#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"

unsigned int nextFreeFileIndex();
unsigned int freeFileIndex = 0;

String cmdPRNT="";
uint8_t chkHeader=99;

byte image_data[40000] = {}; //moreless 14 photos (82.236) (max:86360)
uint32_t img_index=0x00;

// Dev Note: Gamboy camera sends data payload of 640 bytes usually
#define GBP_BUFFER_SIZE 650

/* Serial IO */
// This circular buffer contains a stream of raw packets from the gameboy
uint8_t gbp_serialIO_raw_buffer[GBP_BUFFER_SIZE] = {0};

inline void gbp_packet_capture_loop();

#ifdef ESP32
  TaskHandle_t TaskWrite;
  bool isPrinting = false;
  bool detachRun = false;
#endif
/*******************************************************************************
  Utility Functions
*******************************************************************************/

const char *gbpCommand_toStr(int val) {
  switch (val) {
    case GBP_COMMAND_INIT    : return "INIT";
    case GBP_COMMAND_PRINT   : return "PRNT";
    case GBP_COMMAND_DATA    : return "DATA";
    case GBP_COMMAND_BREAK   : return "BREK";
    case GBP_COMMAND_INQUIRY : return "INQY";
    default: return "?";
  }
}

/*******************************************************************************
  Interrupt Service Routine
*******************************************************************************/

void ICACHE_RAM_ATTR serialClock_ISR(void)
{
  // Serial Clock (1 = Rising Edge) (0 = Falling Edge); Master Output Slave Input (This device is slave)
  #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
    const bool txBit = gpb_serial_io_OnRising_ISR(digitalRead(GB_MOSI));
  #else
    const bool txBit = gpb_serial_io_OnChange_ISR(digitalRead(GB_SCLK), digitalRead(GB_MOSI));
  #endif
    digitalWrite(GB_MISO, txBit ? HIGH : LOW);
}



unsigned int nextFreeFileIndex() {
  #ifdef ESP8266
    for (int i = 1; i < MAX_IMAGES; i++) {
      char path[31];
      sprintf(path, "/d/%05d.txt", i);
      if (!FS.exists(path)) {
        return i;
      }
    }
    return MAX_IMAGES + 1;
  #endif
  #ifdef ESP32
    int totFiles = 0;
    File root = FS.open("/d");
    File file = root.openNextFile();
    while(file){
      if(file){
        totFiles++;
      }
      file = root.openNextFile();
    }
    return totFiles;
  #endif
}



void resetValues() {
  img_index = 0x00; 
  
  memset(image_data, 0x00, sizeof(image_data));

  #ifdef ESP8266
    /*Attach ISR Again*/
    #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
      attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, RISING);  // attach interrupt handler
    #else
      attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, CHANGE);  // attach interrupt handler
    #endif
  #endif   
  #ifdef ESP32
    if (detachRun){
      /*Attach ISR Again*/
      #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
        attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, RISING);  // attach interrupt handler
      #else
        attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, CHANGE);  // attach interrupt handler
      #endif
      detachRun = false;
    }
    showPrinterStats();
    gbp_serial_io_print_done();
  #endif
  
  // Turn LED ON
  digitalWrite(LED_BLINK_PIN, false);
  Serial.println("Printer ready.");
}



#ifdef ESP8266
  void storeData(byte *image_data)
#endif
#ifdef ESP32
  void storeData(void *pvParameters)
#endif
{
  #ifdef ESP8266
    detachInterrupt(digitalPinToInterrupt(GB_SCLK));
  #endif
  #ifdef ESP32
    byte *image_data2 = ((byte*)pvParameters);
  #endif
  
  unsigned long perf = millis();
  char fileName[31];
  oled_msg("Saving...");
  
  sprintf(fileName, "/d/%05d.txt", freeFileIndex);
  digitalWrite(LED_BLINK_PIN, LOW);

  #ifdef ESP8266
    File file = FS.open(fileName, "w");
  #endif
  #ifdef ESP32
    File file = FS.open(fileName, FILE_WRITE);
  #endif

  if (!file) {
    Serial.println("file creation failed");
  }
  
  #ifdef ESP8266
    file.write(image_data, img_index);
  #endif
  #ifdef ESP32
    file.write(image_data2, img_index);
  #endif 
  file.close();

  perf = millis() - perf;
  Serial.printf("File /d/%05d.txt written in %lums\n", freeFileIndex, perf);

  freeFileIndex++;

  #ifdef ESP8266
    // ToDo: Handle percentages
    int percUsed = fs_info();
    if (percUsed > 5) {
      resetValues();
    } else {
      Serial.println("no more space on printer\nrebooting...");
      ESP.restart();
    }  
  #endif
  #ifdef ESP32
    if (freeFileIndex <= MAX_IMAGES) {
      resetValues();      
    } else {
      Serial.println("no more space on printer\nrebooting...");
      ESP.restart();
    }
    vTaskDelete( NULL );
  #endif 
}



// Blink if printer is full.
void full() {
  Serial.println("no more space on printer");

  #ifdef USE_OLED
    oled_msg((String)"Printer is full :-(");
  #endif

  while(true) {
    digitalWrite(LED_BLINK_PIN, LOW);
    delay(1000);
    digitalWrite(LED_BLINK_PIN, HIGH);
    delay(500);
  }
}



void espprinter_setup() {
  // Setup ports
  pinMode(GB_MISO, OUTPUT);
  pinMode(GB_MOSI, INPUT);
  pinMode(GB_SCLK, INPUT);

  /* Default link serial out pin state */
  digitalWrite(GB_MISO, LOW);

  freeFileIndex = nextFreeFileIndex();

  if (freeFileIndex <= MAX_IMAGES) {
    Serial.printf("Next file: /d/%05d.txt\n", freeFileIndex);
  } else {
    full();
  }

  /* Setup */
  gpb_serial_io_init(sizeof(gbp_serialIO_raw_buffer), gbp_serialIO_raw_buffer);

  /* Attach ISR */
  #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
    attachInterrupt( digitalPinToInterrupt(GB_SCLK), serialClock_ISR, RISING);  // attach interrupt handler
  #else
    attachInterrupt( digitalPinToInterrupt(GB_SCLK), serialClock_ISR, CHANGE);  // attach interrupt handler
  #endif
}


#ifdef USE_OLED
  void showPrinterStats() {
    char printed[20];
    #ifdef ESP8266
      int percUsed = fs_info();
      sprintf(printed, "%3d files", freeFileIndex - 1);
      oled_msg(((String)percUsed)+((char)'%')+" remaining",printed);
    #endif
    #ifdef ESP32
      char remain[20];
      sprintf(printed, "%3d printed", freeFileIndex - 1);
      sprintf(remain, "%3d remaining", MAX_IMAGES + 1 - freeFileIndex);
      oled_msg(
        printed,
        remain
      );
    #endif
    oled_drawLogo();
  }
#endif

inline void gbp_packet_capture_loop() {
  /* tiles received */
  static uint32_t byteTotal = 0;
  static uint32_t pktTotalCount = 0;
  static uint32_t pktByteIndex = 0;
  static uint16_t pktDataLength = 0;
  const size_t dataBuffCount = gbp_serial_io_dataBuff_getByteCount();
  if (
    ((pktByteIndex != 0) && (dataBuffCount > 0)) ||
    ((pktByteIndex == 0) && (dataBuffCount >= 6))
  ) {
    const char nibbleToCharLUT[] = "0123456789ABCDEF";
    uint8_t data_8bit = 0;

    // Display the data payload encoded in hex
    for (int i = 0 ; i < dataBuffCount ; i++) {
      #ifdef ESP32
        if (!isPrinting){
          isPrinting = true;
        }
      #endif
      // Start of a new packet
      if (pktByteIndex == 0) {
        pktDataLength = gbp_serial_io_dataBuff_getByte_Peek(4);
        pktDataLength |= (gbp_serial_io_dataBuff_getByte_Peek(5)<<8)&0xFF00;
        chkHeader = gbp_serial_io_dataBuff_getByte_Peek(2);

        if (chkHeader == 1) {
          oled_msg("Receiving data...");
        }

        digitalWrite(LED_BLINK_PIN, HIGH);
      }

      // Print Hex Byte
      data_8bit = gbp_serial_io_dataBuff_getByte();

      if (chkHeader == 2) {
        image_data[img_index] = (byte)data_8bit;
        img_index++;

        cmdPRNT = cmdPRNT+((char)nibbleToCharLUT[(data_8bit>>4)&0xF]) + ((char)nibbleToCharLUT[(data_8bit>>0)&0xF]);
      } else if (chkHeader == 1 || (chkHeader == 4 && pktDataLength > 9)) {
        image_data[img_index] = (byte)data_8bit;
        img_index++;
      }
      
      // Splitting packets for convenience
      if ((pktByteIndex > 5) && (pktByteIndex >= (9 + pktDataLength))) {
        digitalWrite(LED_BLINK_PIN, LOW);
        if (cmdPRNT.length() == 28) {
          if (((int)(cmdPRNT[15]-'0')) > 0) {
            #ifdef ESP8266
              storeData(image_data);
            #endif
            #ifdef ESP32
              if (img_index > 35200) {
                xTaskCreatePinnedToCore(
                                        storeData,            // Task function. 
                                        "storeData",          // name of task. 
                                        10000,                // Stack size of task 
                                        (void*)&image_data,   // parameter of the task 
                                        1,                    // priority of the task 
                                        &TaskWrite,           // Task handle to keep track of created task 
                                        0);                   // pin task to core 0 
              }else{
                gbp_serial_io_print_done();
              }
            #endif
            chkHeader = 99;
          }
          gbp_serial_io_print_done();
          cmdPRNT = "";
        }
        pktByteIndex = 0;
        pktTotalCount++;
      } else {
        pktByteIndex++; // Byte hex split counter
        byteTotal++; // Byte total counter
      }
    }
  }
}

void espprinter_loop() {
  gbp_packet_capture_loop();
  
  // Trigger Timeout and reset the printer if byte stopped being received.
  static uint32_t last_millis = 0;
  uint32_t curr_millis = millis();
  if (curr_millis > last_millis) {
    uint32_t elapsed_ms = curr_millis - last_millis;
    if (gbp_serial_io_timeout_handler(elapsed_ms)) {
      digitalWrite(LED_BLINK_PIN, LOW);
      if (img_index > 0 && isPrinting){
        isPrinting = false;
        detachInterrupt(digitalPinToInterrupt(GB_SCLK));
        detachRun = true;
        xTaskCreatePinnedToCore(
                                storeData,            // Task function. 
                                "storeData",          // name of task. 
                                10000,                // Stack size of task 
                                (void*)&image_data,   // parameter of the task 
                                1,                    // priority of the task 
                                &TaskWrite,           // Task handle to keep track of created task 
                                0);                   // pin task to core 0 
      }
    }
  }
  last_millis = curr_millis;  
}
