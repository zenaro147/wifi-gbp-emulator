#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"

unsigned int nextFreeFileIndex();
unsigned int freeFileIndex = 0;

uint8_t cmdPRNT=0x00;
uint8_t chkHeader=99;

byte image_data[12000] = {}; //moreless 14 photos (82.236) -- ESP32 can handle up to 500 images without the app open or 400 with the app
byte img_tmp[12000] = {};
uint32_t img_index=0x00;

// Dev Note: Gamboy camera sends data payload of 640 bytes usually
#define GBP_BUFFER_SIZE 650

/* Serial IO */
// This circular buffer contains a stream of raw packets from the gameboy
uint8_t gbp_serialIO_raw_buffer[GBP_BUFFER_SIZE] = {0};

inline void gbp_packet_capture_loop();

TaskHandle_t TaskWrite;
bool isWriting = false;

bool setMultiPrint = false;
unsigned int totalMultiImages = 1;

uint8_t dtpck = 0;
uint8_t inqypck = 0;

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

/*******************************************************************************
  Check Next file available
*******************************************************************************/
unsigned int nextFreeFileIndex() {
  int totFiles = 0;
  File root = FSYS.open("/d");
  File file = root.openNextFile();
  while(file){
    if(file){
      totFiles++;
    }
    file = root.openNextFile();
  }
  return totFiles + 1;
}

/*******************************************************************************
  Show printer status on display
*******************************************************************************/
#ifdef USE_OLED
void showPrinterStats() {
  char printed[20];
  char remain[20];
  sprintf(printed, "%3d printed", freeFileIndex - 1);
  sprintf(remain, "%3d remaining", MAX_IMAGES + 1 - freeFileIndex);
  oled_msg(
    printed,
    remain
  );
  oled_drawLogo();
}
#endif

/*******************************************************************************
  Blink if printer is full.
*******************************************************************************/
void full() {
  Serial.println("no more space on printer");
  digitalWrite(LED_BLINK_PIN, HIGH);
  #ifdef USE_OLED
    oled_msg("Printer full","Rebooting...");
  #endif
  delay(3000);
  ESP.restart();
}

/*******************************************************************************
  Clear Variables before writing
*******************************************************************************/
void resetValues() {
  img_index = 0x00;     
    
  // Turn LED ON
  digitalWrite(LED_BLINK_PIN, false);
  
  chkHeader = 99; 
  
  dtpck = 0x00;
  inqypck = 0x00;
  cmdPRNT = 0x00;
  isWriting = false;
}

/*******************************************************************************
  Write received data into a file
*******************************************************************************/
void storeData(void *pvParameters){
  unsigned long perf = millis();
  byte *image_data2 = ((byte*)pvParameters);
  int img_index2=img_index;
  char fileName[31];
  
  digitalWrite(LED_BLINK_PIN, LOW);
  
  #ifdef USE_OLED
    oled_msg("Saving...");
  #endif
  
  if(setMultiPrint || totalMultiImages > 1){
    sprintf(fileName, "/t/%05d_%05d.txt", freeFileIndex,totalMultiImages);
  }else{
    sprintf(fileName, "/d/%05d.txt", freeFileIndex);
  }
  
  File file = FSYS.open(fileName, FILE_WRITE);
  if (!file) {
    Serial.println("file creation failed");
  }
  file.write(image_data2, img_index2);
  file.close();
  
  perf = millis() - perf;
  if(setMultiPrint || totalMultiImages > 1){
    Serial.printf("File /t/%05d_%05d.txt written in %lums\n", freeFileIndex,totalMultiImages,perf);
  }else{
    Serial.printf("File /d/%05d.txt written in %lums\n", freeFileIndex, perf);
  }
  
  if (freeFileIndex < MAX_IMAGES) {
    if(!setMultiPrint && totalMultiImages <= 1){
      freeFileIndex++; 
    }else if (setMultiPrint){    
      totalMultiImages++;
    }
    resetValues();
    vTaskDelete(NULL); 
  } else {
    Serial.println("no more space on printer\nrebooting...");
    full();
  }    
}

/*******************************************************************************
  Merge multiple files into one single file
*******************************************************************************/
void gpb_mergeMultiPrint(void *pvParameters){
  byte inqypck[10] = {B10001000, B00110011, B00001111, B00000000, B00000000, B00000000, B00001111, B00000000, B10000001, B00000000};
  img_index = 0;
  memset(image_data, 0x00, sizeof(image_data));
  Serial.println("Merging Files");

  char path[31];
  for (int i = 1 ; i <= totalMultiImages ; i++){
    sprintf(path, "/t/%05d_%05d.txt", freeFileIndex,i);
    //Read File
    File file = FSYS.open(path);
    while(file.available()){
      image_data[img_index] = ((byte)file.read());
      img_index++;
    }
    file.close();
    FSYS.remove(path);
    
    //Write File
    sprintf(path, "/d/%05d.txt", freeFileIndex);
    if(i == 1){
      file = FSYS.open(path, FILE_WRITE);
    }else{
      file = FSYS.open(path, FILE_APPEND);
    }
    file.write(image_data,img_index);
    file.write(inqypck, 10);
    file.close();
    
    memset(image_data, 0x00, sizeof(image_data));
    img_index = 0;
  } 
  
  setMultiPrint = false;
  totalMultiImages = 1;
  
  Serial.printf("File %s written", path);
  if (freeFileIndex < MAX_IMAGES) {
    freeFileIndex++; 
    resetValues();
  } else {
    Serial.println("no more space on printer\nrebooting...");
    full();
  }

  #ifdef USE_OLED
    showPrinterStats();
  #endif 
  
  vTaskDelete(NULL); 
}

/*******************************************************************************
  Main loop to capture data from the Gameboy
*******************************************************************************/
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

        chkHeader = (int)gbp_serial_io_dataBuff_getByte_Peek(2);
        
//        Serial.print("// ");
//        Serial.print(pktTotalCount);
//        Serial.print(" : ");
//        Serial.println(gbpCommand_toStr(gbp_serial_io_dataBuff_getByte_Peek(2)));

        switch (chkHeader) {
          case 1:
            cmdPRNT = 0x00;
            break;
          case 2:          
            break;
          case 4:
            ////////////////////////////////////////////// FIX for merge print in McDonald's Monogatari : Honobono Tenchou Ikusei Game and Nakayoshi Cooking Series 5 : Cake o Tsukurou //////////////////////////////////////////////
            if (pktDataLength > 0){
              //Count how many data packages was received before sending the print command
              dtpck++;
            }
            ////////////////////////////////////////////// FIX for merge print in McDonald's Monogatari : Honobono Tenchou Ikusei Game and Nakayoshi Cooking Series 5 : Cake o Tsukurou //////////////////////////////////////////////
            break;
          case 15:
            ////////////////////////////////////////////// FIX for Tales of Phantasia //////////////////////////////////////////////
            if (totalMultiImages > 1 && !isWriting && !setMultiPrint){
              inqypck++;
              if(inqypck > 20){
                //Force to write the saves images  
                xTaskCreatePinnedToCore(gpb_mergeMultiPrint,    // Task function. 
                                        "mergeMultiPrint",      // name of task. 
                                        10000,                  // Stack size of task 
                                        NULL,                   // parameter of the task 
                                        1,                      // priority of the task 
                                        &TaskWrite,             // Task handle to keep track of created task 
                                        0);                     // pin task to core 0               
                inqypck=0;
              }
            }
            ////////////////////////////////////////////// FIX for Tales of Phantasia //////////////////////////////////////////////
            break;
          default:
            break;
        }        
        digitalWrite(LED_BLINK_PIN, HIGH);
      }

      // Print Hex Byte
      data_8bit = gbp_serial_io_dataBuff_getByte();
//      Serial.print((char)nibbleToCharLUT[(data_8bit>>4)&0xF]);
//      Serial.print((char)nibbleToCharLUT[(data_8bit>>0)&0xF]);

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
        digitalWrite(LED_BLINK_PIN, LOW);
        if (chkHeader == 2 && !isWriting) {
          isWriting=true;
          if((cmdPRNT == 0 || ((cmdPRNT == 3 && dtpck == 1) || (cmdPRNT == 1 && dtpck == 6))) && !setMultiPrint){
            setMultiPrint=true;
            dtpck=0x00;
          }else if(cmdPRNT > 0 && setMultiPrint){
            setMultiPrint=false;
          }
          memcpy(img_tmp,image_data,12000);
          xTaskCreatePinnedToCore(storeData,            // Task function. 
                                  "storeData",          // name of task. 
                                  10000,                // Stack size of task 
                                  (void*)&img_tmp,   // parameter of the task 
                                  1,                    // priority of the task 
                                  &TaskWrite,           // Task handle to keep track of created task 
                                  0);                   // pin task to core 0 
          memset(image_data, 0x00, sizeof(image_data));
        }
//        Serial.println("");
        pktByteIndex = 0;
        pktTotalCount++;
      } else {
//        Serial.print((char)' ');
        pktByteIndex++; // Byte hex split counter
        byteTotal++; // Byte total counter
      }
    }
  }
}

/*******************************************************************************
  Init ESP printer
*******************************************************************************/
void espprinter_setup() {
  // Setup ports
  pinMode(GB_MISO, OUTPUT);
  pinMode(GB_MOSI, INPUT);
  pinMode(GB_SCLK, INPUT);

  /* Default link serial out pin state */
  digitalWrite(GB_MISO, LOW);

  freeFileIndex = nextFreeFileIndex();
  
  if (freeFileIndex > MAX_IMAGES) {
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

/*******************************************************************************
  Loop tasks from ESP printer
*******************************************************************************/
void espprinter_loop() {
  gbp_packet_capture_loop();
  
  // Trigger Timeout and reset the printer if byte stopped being received.
  static uint32_t last_millis = 0;
  uint32_t curr_millis = millis();
  if (curr_millis > last_millis) {
    uint32_t elapsed_ms = curr_millis - last_millis;
    if (gbp_serial_io_timeout_handler(elapsed_ms)) {
      Serial.println("Printer Timeout");
      digitalWrite(LED_BLINK_PIN, LOW);   

      if(!setMultiPrint && totalMultiImages > 1 && !isWriting){
        #ifdef USE_OLED
          oled_msg("Long Print detected","Merging Files...");
        #endif
        isWriting = true;
        xTaskCreatePinnedToCore(gpb_mergeMultiPrint,    // Task function. 
                        "mergeMultiPrint",              // name of task. 
                        10000,                          // Stack size of task 
                        NULL,                           // parameter of the task 
                        1,                              // priority of the task 
                        &TaskWrite,                     // Task handle to keep track of created task 
                        0);                             // pin task to core 0  
      }
      
      #ifdef USE_OLED
        if(!isWriting){
          showPrinterStats();
        }
      #endif 
    }
  }
  last_millis = curr_millis; 
}
