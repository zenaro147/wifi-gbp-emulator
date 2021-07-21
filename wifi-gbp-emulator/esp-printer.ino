#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"

unsigned int nextFreeFileIndex();
unsigned int freeFileIndex = 0;

uint8_t cmdPRNT=0x00;
uint8_t chkHeader=99;

byte image_data[30000] = {}; //moreless 5 photos (29.370)
uint32_t img_index=0x00;

// Dev Note: Gamboy camera sends data payload of 640 bytes usually
#define GBP_BUFFER_SIZE 650

/* Serial IO */
// This circular buffer contains a stream of raw packets from the gameboy
uint8_t gbp_serialIO_raw_buffer[GBP_BUFFER_SIZE] = {0};

inline void gbp_packet_capture_loop();

bool isWriting = false;

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

#ifdef ESP8266
void ICACHE_RAM_ATTR serialClock_ISR(void)
#else
void serialClock_ISR(void)
#endif
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
  for (int i = 1; i < MAX_IMAGES; i++) {
    char path[31];
    sprintf(path, "/d/%05d.txt", i);
    if (!FS.exists(path)) {
      return i;
    }
  }
  return MAX_IMAGES + 1;
}

void resetValues() {
  
  memset(image_data, 0x00, sizeof(image_data));
  img_index = 0x00;
  
  // Turn LED ON
  digitalWrite(LED_BLINK_PIN, false);
  Serial.println("Printer ready.");
  
  cmdPRNT = 0x00;
  chkHeader = 99;  
  isWriting = false;
  
}

void storeData(byte *image_data) {
  unsigned long perf = millis();
  char fileName[31];
  sprintf(fileName, "/d/%05d.txt", freeFileIndex);

  digitalWrite(LED_BLINK_PIN, LOW);


  detachInterrupt(digitalPinToInterrupt(GB_SCLK));
  
  File file = FS.open(fileName, "w");
  if (!file) {
    Serial.println("file creation failed");
  }
  // for (int i = 0 ; i < img_index ; i++){
  //   file.print((char)image_data[i]);
  // }
  file.write(image_data, img_index);
  file.close();
  
  /* Attach ISR Again*/
  #ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
    attachInterrupt( digitalPinToInterrupt(GB_SCLK), serialClock_ISR, RISING);  // attach interrupt handler
  #else
    attachInterrupt( digitalPinToInterrupt(GB_SCLK), serialClock_ISR, CHANGE);  // attach interrupt handler
  #endif
  

  perf = millis() - perf;
  Serial.printf("File /d/%05d.txt written in %lums\n", freeFileIndex, perf);

  freeFileIndex++;
  // ToDo: Handle percentages
  int percUsed = fs_info();
  if (percUsed > 5) {
    resetValues();
  } else {
    Serial.println("no more space on printer\nrebooting...");
    full();
  }
}

// Blink if printer is full.
void full() {
  Serial.println("no more space on printer");

  #ifdef USE_OLED
    oled_msg((String)"Printer is full :-(","Rebooting...");
  #endif

  delay(3000);
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
          case 15:
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
        digitalWrite(LED_BLINK_PIN, LOW);
        
        if (chkHeader == 2) {
          if (cmdPRNT > 0 && !isWriting) {
            gbp_serial_io_print_set();  
            isWriting=true;
            //Call Write task
            storeData(image_data);
          }else{
              cmdPRNT = 0x00;
              chkHeader = 99;
              isWriting = false;
              gbp_serial_io_print_done();
          }
        }else{
          if(chkHeader == 99 && !isWriting && cmdPRNT == 0 && gbp_serial_io_should_print()){
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
    }
  }

  last_millis = curr_millis;
}
