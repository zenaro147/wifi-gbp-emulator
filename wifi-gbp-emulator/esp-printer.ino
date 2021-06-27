#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"

unsigned int nextFreeFileIndex();
unsigned int freeFileIndex = 0;

uint8_t cmdPRNT=0x00;
uint8_t chkHeader=99;

byte image_data[86000] = {}; //moreless 14 photos (82.236) (max:86360)
uint32_t img_index=0x00;

// Dev Note: Gamboy camera sends data payload of 640 bytes usually
#define GBP_BUFFER_SIZE 650

/* Serial IO */
// This circular buffer contains a stream of raw packets from the gameboy
uint8_t gbp_serialIO_raw_buffer[GBP_BUFFER_SIZE] = {0};

inline void gbp_packet_capture_loop();

File file;
TaskHandle_t TaskWrite;
bool isWriting = false;
bool detachRun = false;
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
  int totFiles = 0;
  File root = FS.open("/d");
  file = root.openNextFile();
  while(file){
    if(file){
      totFiles++;
    }
    file = root.openNextFile();
  }
  return totFiles + 1;
}



void resetValues() {
  memset(image_data, 0x00, sizeof(image_data));
  img_index = 0x00;    
  
  /*if (detachRun){
    //Attach ISR Again
    #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
      attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, RISING);
    #else
      attachInterrupt(digitalPinToInterrupt(GB_SCLK), serialClock_ISR, CHANGE); 
    #endif
    detachRun = false; 
  }*/   
    
  // Turn LED ON
  digitalWrite(LED_BLINK_PIN, false);
  Serial.println("Printer ready.");

  Serial.print("yy-");  
  Serial.print(chkHeader);
  Serial.print("-");
  Serial.print(cmdPRNT);
  Serial.print("-");
  Serial.print(isWriting);
  Serial.print("-");
  Serial.println(gbp_serial_io_should_print());
  
  cmdPRNT = 0x00;
  chkHeader = 99;  
  isWriting = false;
  //delay(200);
  //gbp_serial_io_print_done();
  
  Serial.print("zz-");  
  Serial.print(chkHeader);
  Serial.print("-");
  Serial.print(cmdPRNT);
  Serial.print("-");
  Serial.print(isWriting);
  Serial.print("-");
  Serial.println(gbp_serial_io_should_print());
}

void storeData(void *pvParameters)
{
  byte *image_data2 = ((byte*)pvParameters);
  
  unsigned long perf = millis();
  char fileName[31];
  
  #ifdef USE_OLED
    oled_msg("Saving...");
  #endif
  
  sprintf(fileName, "/d/%05d.txt", freeFileIndex);
  digitalWrite(LED_BLINK_PIN, LOW);

  file = FS.open(fileName, "w");
  if (!file) {
    Serial.println("file creation failed");
  }
  file.write(image_data2, img_index);
  file.close();
  
  perf = millis() - perf;
  Serial.printf("File /d/%05d.txt written in %lums\n", freeFileIndex, perf);

  freeFileIndex++;

  uint8_t percUsed = fs_info();
  if (percUsed > 5) { 
    resetValues();
    //gbp_serial_io_print_done();
    vTaskDelete(NULL); 
  } else {
    Serial.println("no more space on printer\nrebooting...");
    full();
  }    
}

// Blink if printer is full.
void full() {
  Serial.println("no more space on printer");
  digitalWrite(LED_BLINK_PIN, HIGH);
  ESP.restart();
}

void espprinter_setup() {
  // Setup ports
  pinMode(GB_MISO, OUTPUT);
  pinMode(GB_MOSI, INPUT);
  pinMode(GB_SCLK, INPUT);

  /* Default link serial out pin state */
  digitalWrite(GB_MISO, LOW);

  freeFileIndex = nextFreeFileIndex();

  int percUsed = fs_info();
  
  if (percUsed < 5) {
    Serial.println("no more space on printer\nrebooting...");
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
  int percUsed = fs_info();
  
  sprintf(printed, "%3d files", freeFileIndex - 1);
  oled_msg(((String)percUsed)+((char)'%')+" remaining",printed);
  
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
      // Start of a new packet
      if (pktByteIndex == 0) {
        pktDataLength = gbp_serial_io_dataBuff_getByte_Peek(4);
        pktDataLength |= (gbp_serial_io_dataBuff_getByte_Peek(5)<<8)&0xFF00;
                     

        switch ((int)gbp_serial_io_dataBuff_getByte_Peek(2)) {
          case 1:
          case 2:
          case 4:
            chkHeader = (int)gbp_serial_io_dataBuff_getByte_Peek(2);
            break;
          default:
            break;
        }
                
        digitalWrite(LED_BLINK_PIN, HIGH);
      }

      // Print Hex Byte
      data_8bit = gbp_serial_io_dataBuff_getByte();

      if (!isWriting){
        if (chkHeader == 1 || chkHeader == 2 || chkHeader == 4){
          image_data[img_index] = (byte)data_8bit;
          img_index++;
          if (chkHeader == 2 && pktByteIndex == 7) { 
            cmdPRNT = (int)((char)nibbleToCharLUT[(data_8bit>>0)&0xF])-'0';
          } 
        }
      }
      
      // Splitting packets for convenience
      if ((pktByteIndex > 5) && (pktByteIndex >= (9 + pktDataLength))) {
        #ifdef USE_OLED
          if (!isWriting){
            oled_msg("Receiving...");
          }
        #endif
        Serial.print("xx-");  
        Serial.print(chkHeader);
        Serial.print("-");
        Serial.print(cmdPRNT);
        Serial.print("-");
        Serial.print(isWriting);
        Serial.print("-");
        Serial.println(gbp_serial_io_should_print());
          
        digitalWrite(LED_BLINK_PIN, LOW);
        if (chkHeader == 2) {
          Serial.print("aa-");  
          Serial.print(chkHeader);
          Serial.print("-");
          Serial.print(cmdPRNT);
          Serial.print("-");
          Serial.print(isWriting);
          Serial.print("-");
          Serial.println(gbp_serial_io_should_print()); 
          if (cmdPRNT > 0 && !isWriting /*&& img_index > 35200*/) {
            gbp_serial_io_print_set();  
            Serial.print("bb-");  
            Serial.print(chkHeader);
            Serial.print("-");
            Serial.print(cmdPRNT);
            Serial.print("-");
            Serial.print(isWriting);
            Serial.print("-");
            Serial.println(gbp_serial_io_should_print());
            isWriting=true;
            xTaskCreatePinnedToCore(storeData,            // Task function. 
                                    "storeData",          // name of task. 
                                    10000,                // Stack size of task 
                                    (void*)&image_data,   // parameter of the task 
                                    1,                    // priority of the task 
                                    &TaskWrite,           // Task handle to keep track of created task 
                                    0);                   // pin task to core 0 
          }else{
              Serial.print("cc-");  
              Serial.print(chkHeader);
              Serial.print("-");
              Serial.print(cmdPRNT);
              Serial.print("-");
              Serial.print(isWriting);
              Serial.print("-");
              Serial.println(gbp_serial_io_should_print()); 
              cmdPRNT = 0x00;
              chkHeader = 99;
              isWriting = false;
              delay(200);
              gbp_serial_io_print_done();
          }
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
      #ifdef USE_OLED
        showPrinterStats();
      #endif 
      /*chkHeader = 99;
      freeFileIndex++;*/
    }
  }
  last_millis = curr_millis;  
}
