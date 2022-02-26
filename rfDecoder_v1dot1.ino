// _____________________________________________________________________________________________
/* Import the SD Fat Library to work with .csv files. 
 *  This an old version (1.1.4) unzipped from November 2020 with multiple dependencies.
 *  A new version imported might look different and have less explicit dependencies. 
 */
#include <BlockDriver.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <sdios.h>
#include <SysCall.h>
// Import the SPI library to communicate with the SD Breakout Board.
#include <SPI.h>
// Import the required library to communicate with the Radio Receiver. 
#include <RCSwitch.h>

// Import the Libraries for the graphic display
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library 
// --------------------------------------------------------------------------------------------


// ____________________________________________________________________________________________
// Define the pins being used by the Elegoo Shield. 
#define USE_Elegoo_SHIELD_PINOUT
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define SD_CS 53 // Required for the additional SPI, aka the breakout board. 
/* The control pins for the LCD can be assigned to any digital or
*     analog pins but the shield has a fixed pin declaration and it 
*     is nice to see it in explicit form. 
*     Elegoo_TFTLCD tft; also works as a replacement to create the instance object. 
*/
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
// --------------------------------------------------------------------------------------------

// ____________________________________________________________________________________________
// Create the instance object for the RC switch library. 
RCSwitch mySwitch = RCSwitch();
// Define the interruptPin, this is the data transmission line for the receiver.
const byte interruptPin = 4; // Note that on Arduino Mega, this corresponds the physical pin 19. 
// Initialize the RF variables to their designated data types.  
unsigned long decimal ; 
unsigned int length ;
unsigned int pulseTime ;
unsigned int protocol;
const char* b;

// Initialize the booleans that act as keys for blocks of code.  
bool newSignal = false; 
bool noSignal = true; 
bool pastSignal = false;
bool sdStatus; 

// Construct instance of the SdFat library
SdFat SD;
// Define the file object which calls upon the SD library's functions. 
File binMurph; 
// You can change the base name of each .csv here so you know which remote's codes you have.
#define FILE_BASE_NAME "etekcity"
// -------------------------------------------------------------------------------------------

// *******************************************************************************************
// ************************************* Begin Set-Up ****************************************
// *******************************************************************************************
void setup(void) {
  /*
   * This block of code came equipped with the TFT, although the serial monitor is not used
   *      in this project since it is portable, it could be useful to keep for debugging
   *      down the road.
   */
   // ----------------------------------------------------------------------------------------
Serial.begin(9600);

#ifdef USE_Elegoo_SHIELD_PINOUT
  Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
#endif

Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());
tft.reset();

   uint16_t identifier = tft.readID();
   if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else if(identifier==0x1111)
  {     
      identifier=0x9328;
       Serial.println(F("Found 0x9328 LCD driver"));
  }
  else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9328;
  
  }
  // -----------------------------------------------------------------------------------------
  
  tft.begin(identifier);  // Begin the TFT Object

  // ---------------------------- Establish the status of the SD card. -----------------------
  pinMode(SD_CS, OUTPUT); // Default for Chip Select Pin is to be an output for the SD Library
  // Determine what to do if the Chip Select pin is not able to initialize. 
  if (!SD.begin(SD_CS)) {
    // Set the boolean of sdStatus to FALSE to disable certain tasks in the loop section. 
    sdStatus = false;
    // Populate the screen with a message to the user letting them know that SD card is Off
    tft.fillScreen(BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(RED); tft.setTextSize(3);
    tft.println("   ALERT");
    tft.println("SD Card Read");
    tft.println("   ERROR");
    tft.println();
    tft.println();
    tft.println("Recording is");
    tft.println("  Offline   ");
    // Allow the user to understand what has happened but allow decoding to continue. 
    delay(5000); 
  }
  else {
    // If the chip select pin initializes, let the user know and keep sdStatus as TRUE.
    sdStatus = true; 
    tft.fillScreen(BLACK);
    tft.setCursor(0,0); 
    tft.setTextColor(WHITE); tft.setTextSize(3);
    tft.println("SD Card is");
    tft.println("   Ready  ");
    tft.println();
    tft.println();
    tft.println("Recording is");
    tft.println("   Online   ");

    delay(5000);
    // Initialize a .csv file with a base name and add extension such as Remote 1 if needed.
    char filename[] = FILE_BASE_NAME "_R1.csv";
    // Define our instance object by calling on an SD function for the file. 
    binMurph = SD.open(filename, FILE_WRITE);
    // Generate the first three columns within the .csv file. 
    binMurph.print(", ");
    binMurph.print("Binary Code");
    binMurph.print(", ");
    binMurph.print("Pulse Length [micro-sec]");
    binMurph.print(", ");
    binMurph.print("Protocol");
    binMurph.println();
  }
  // --------------------------------------------------------------------------------------------
  
  // Enable the object for the RF Receiver, interrupt 4 => receiver data is on pin 19 for MEGA.
  mySwitch.enableReceive(interruptPin);  
}
// **********************************************************************************************
// ***********************************  End of the Set-Up ***************************************
// **********************************************************************************************


// **********************************************************************************************
// ***********************************   Begin the Loop  ****************************************
// **********************************************************************************************
void loop() {
  tft.fillScreen(BLACK);
  castNet(); 
  if (noSignal == false) {
    
    while (pastSignal == true) {
      // Display the captured signal to the screen until a new transmission is received.  
      showSignal();
      if (mySwitch.available()) {
        decryptAgain();     
        if (sdStatus) {
          record2File();
        }
        newSignal = true; // Provide the key to enter the new signal block. 
        pastSignal = false; // Get out of while loop to alert operator of new signal. 
      }
    }

    if (newSignal == true) {
      alertOp(); 
      newSignal = false; // New signal is no longer true, exit. 
      pastSignal = true; // Get Back into the while loop to display the results until a new signal. 
    }
  }   
}
// **********************************************************************************************
// *************************************  End of the Loop ***************************************
// **********************************************************************************************

/* ****************************************************************************
 *  Define the first function within the loop that looks out for the first  
 *       signal and then shuts off after providing the key to the first
 *       if block within the loop. 
 * ***************************************************************************/
void castNet() {
  while (noSignal == true) {
    // Tell the operator the state of the device. 
    // Waiting for a signal to display and record if SD initialized. 
    tft.setCursor(0, 0);
    tft.setTextColor(RED);  tft.setTextSize(3);
    tft.println("433.92 MHz RF");
    tft.println("   Decoder  "); 
    tft.println();
    tft.println();
    
    tft.setTextColor(GREEN);  tft.setTextSize(3);
    tft.println(" Waiting for");
    tft.println("  Signal ...");
     
    // If the receiver detects a signal, mySwitch.available() == true
    if (mySwitch.available()) {
      // Populate the variables with the required data for automation.
      // First, we will get the sequence encoded within 433.29 MHz
      decimal = mySwitch.getReceivedValue();
      length = mySwitch.getReceivedBitlength();
      // Convert the decimal into a binary sequence. 
      b = dec2binWzerofill(decimal, length);
      // Pulse Time is given in [micro-seconds] 
      pulseTime = mySwitch.getReceivedDelay();
      // Obtain the Protocol, admittedly, I do not know what this means. 
      protocol = mySwitch.getReceivedProtocol(); 
      // Reset the Receiver after populating the variables.
      mySwitch.resetAvailable();
      // Let the operator know that the first signal has been detected. 
      tft.fillScreen(BLACK);
      tft.setCursor(0,0); 
      tft.setTextColor(BLUE); tft.setTextSize(3);
      tft.println("    First   ");
      tft.println("Transmission");
      tft.println("  Received  "); 
      delay(2000);
      tft.fillScreen(BLACK); 

      // If the .csv is available, then record the data to it. 
      if (sdStatus) {
        binMurph.print(", ");
        binMurph.print(b);
        binMurph.print(", ");
        binMurph.print(pulseTime);
        binMurph.print(", ");
        binMurph.print(protocol);
        binMurph.print(", ");
        binMurph.println();
        binMurph.flush();  
      }
      // Provide the access key to another while block. 
      pastSignal = true; 
      // Provide the key to the first if block and exit out of this while loop. 
      noSignal = false; 
    }
  }
}

/* ************************************************************************
 *  Define the second function which is used inside castNet() as well as .
 *       the loop. This block is designated to return a modified pointer to   
 *       a constant character until a new signal is received, in this case,   
 *       a decimal sequence which is converted to a binary.
 * ***********************************************************************/
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64]; 
  unsigned int i=0;

  while (Dec > 0) {
    bin[32+i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}

/* **************************************************************************
 * Define the third function which is used to print the decyrpted signal.
 * *************************************************************************/
void showSignal() {
     tft.setCursor(0,0); 
     tft.setTextColor(GREEN);  tft.setTextSize(2);
     tft.println("The Binary Code:");
     tft.setTextColor(WHITE); tft.setTextSize(1);
     tft.println(b);

     tft.setTextColor(GREEN);  tft.setTextSize(2);
     tft.println("Pulse [micro-sec]:");
     tft.setTextColor(WHITE); tft.setTextSize(1);
     tft.println(pulseTime); 

     tft.setTextColor(GREEN);  tft.setTextSize(2);
     tft.println("The Protocol:  ");
     tft.setTextColor(WHITE); tft.setTextSize(1);
     tft.print(protocol);
     tft.println();
}

 /* *************************************************************************
  * Define the fourth function which simply populates the necessary variables
  *      for the decypted signal again after the first signal was detected. 
  * ************************************************************************/
void decryptAgain() {
     decimal = mySwitch.getReceivedValue();
     length = mySwitch.getReceivedBitlength();
     pulseTime = mySwitch.getReceivedDelay();
     b = dec2binWzerofill(decimal, length);
     protocol = mySwitch.getReceivedProtocol();
     mySwitch.resetAvailable();
}

/* *************************************************************************
 *  Define the fifth function which simply records the decrypted signal 
 *       to the .csv file you had defined if it is available. 
 *  ************************************************************************/
void record2File() {
     binMurph.print(", ");
     binMurph.print(b);
     binMurph.print(", ");
     binMurph.print(pulseTime);
     binMurph.print(", ");
     binMurph.print(protocol);
     binMurph.print(", ");
     binMurph.println(); 
     binMurph.flush();
}

/* **************************************************************************
 *  Define the sixth function which prints to the screen letting the operator
 *       know a new decrypted signal is about to be displayed.  
 * *************************************************************************/
void alertOp() {
     tft.fillScreen(BLACK); 
     tft.setCursor(0,0);
     tft.setTextColor(RED);  tft.setTextSize(4);
     tft.println("   ALERT! ");
     tft.println();
     tft.println();
      
     tft.setTextColor(BLUE);  tft.setTextSize(3);
     tft.println("   INCOMING");
     tft.println("    SIGNAL "); 
     delay(3000);
}
// ---------------------------------------------------------------------------
