
/*
   PROJECT NAME : SMART GATE KEEPER
   AUTHOR       : SIJI MARIAM THOMAS
   TEAM MEMBERS : AKSHAY NAG NAGENDRAN
                  TRINADH CHADALAVADA
   DATE CREATED : APRIL 10, 2022

   Uses MIFARE RFID card using RFID-RC522 reader
   Uses MFRC522 - Library
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             6         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15

  Arduino NANO is used here as the Board.
*/

#include <SPI.h>
#include <MFRC522.h>
#include "Adafruit_Keypad.h"
#include <LiquidCrystal.h>


#define RST_PIN         6           // Configurable, see typical pin layout above
#define SS_PIN          53          // Configurable, see typical pin layout above

#define MASTER  {0xE5, 0x19, 0xF3, 0x30}   // UID of the MASTER Card
#define MASTERPASS "1n7YsKAv"              // Password saved in Master RFID Card
#define REGULARPASS "OK8va6so"             // Password saved in Regular RFID cards
#define PASSLEN 8                          // Password Length
#define MASTERKEYPASS "1595A"              // Master password to enter programming mode for keypad
#define PIR_INPUT  28                      // Digital Input Pin for PIR Sensor 
#define LED_RED    29                      // Digital Output Pin for Red ACCESS DENIED Led
#define LED_GREEN  30                      // Digital Output Pin for Green ACCESS GRANTED Led
#define LED_BLUE   31                      // Digital Output Pin for Blue PROGRAMMING MODE Led
#define LED_SYSOP  32                      // Digital Output Pin for Yellow SYSTEM OPERATIONAL Led

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

int byte_comp(byte* , byte* , byte ); // Byte compare function used to compare UID
int checkPass(char*);                 // Read password from card memory and compare with given password
void displayUID(byte*, byte);         // Display UID to Serial Monitor
void keypadCheck(void);               // Function Checks the Keypad input to Password.
void bootUP();                        // Boot up Sequence

// Some Initializations
byte block;
byte len;
MFRC522::StatusCode status;
MFRC522::MIFARE_Key key;

// Status Flags required 
static byte prevUID[] = {0x00, 0x00, 0x00, 0x00};
static int padCount = 0;
static int tagCount = 0;
static int masterFlag = 0;
static int keyMasterFlag = 0;


static char keyPassword[5] = "75128";                   // Iniital Regular password for Keypad

const byte ROWS = 4; // rows
const byte COLS = 4; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {11, 10, 9, 8}; //connect to the column pinouts of the keypad

// LCD variables
const int rs = 22, en = 23, d4 = 24, d5 = 25, d6 = 26, d7 = 27;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//some initialisations
byte password[5] = "";
int i = 0;
keypadEvent e;
//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);





//*****************************************************************************************//
void setup() {
  Serial.begin(9600);                                      // Initialize serial communications with the PC
  SPI.begin();                                             // Init SPI bus
  mfrc522.PCD_Init();                                      // Init MFRC522 card
  Serial.println(F("Booting Smart Gate Keeper..."));    // Shows in serial that it is ready to read
  lcd.begin(16, 4);
  pinMode(LED_RED , OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_SYSOP, OUTPUT);
  pinMode(PIR_INPUT, INPUT);
// Runs the boot up sequence
  bootUP();

  customKeypad.begin();
}

//*****************************************************************************************//
void loop() {

  int pirRead = digitalRead(PIR_INPUT);   //Variable to store the PIR Sensor input

  if (pirRead) {
    digitalWrite(LED_SYSOP, HIGH);
    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.

    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    // Some variables we need

    customKeypad.tick();

    //-------------------------------------------

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial()) {
      if (!customKeypad.available()) {
        return;
      } else {

        // Runs the Keypad code
        keypadCheck();
      }
      return;
    }
    //-------------------------------------------
    displayUID(mfrc522.uid.uidByte, mfrc522.uid.size); // displays UID of RFID on to the serial monitor

    byte master[] = MASTER;               // Variable holding Master UID
    char masterPass[8] = MASTERPASS;      // Variable containing Master Password
    char regularPass[8] = REGULARPASS;    // Variable containing Regular Password

    if (byte_comp(master, mfrc522.uid.uidByte, mfrc522.uid.size) && checkPass(masterPass)) {
      Serial.print("Master Card Detected\n");
      
      lcd.clear();
      lcd.write("Master FOB");
      lcd.setCursor(0, 1);
      lcd.write("Detected..");
      delay(2000);
      lcd.clear();

      masterFlag = !masterFlag;
      tagCount = 0;
      
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        prevUID[i] = 0;
      }
      if (!masterFlag) {
        Serial.print("Exit Programming Mode\n");
        Serial.println();
        
        lcd.clear();
        lcd.write("Exit");
        delay(2000);
        lcd.clear();
        digitalWrite(LED_BLUE, LOW);
      }
      else {
        Serial.print("Programming Mode\n");
        Serial.println();
        
        lcd.clear();
        digitalWrite(LED_BLUE, HIGH);
        lcd.write("Programming Mode");
      }
    }
    else if (checkPass(regularPass) && !masterFlag) {
      Serial.print("FOB Access Granted\n");
      Serial.println();
      
      digitalWrite(LED_GREEN, HIGH);
      lcd.clear();
      lcd.write("Access Granted");
      delay(2000);
      lcd.clear();
      digitalWrite(LED_GREEN, LOW);
      
      tagCount = 0;
     
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        prevUID[i] = 0;
      }
    }
    else if (masterFlag) {
      Serial.print("New CArd Detected\n");
      
      lcd.clear();
      lcd.write("Programming Mode");
      lcd.setCursor(0, 1);
      lcd.write("New Card");
      delay(2000);
      lcd.clear();
      lcd.write("Programming Mode");

      // Following sequence of steps programs the regular password on to the RFID in Programming mode
      displayUID(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.print("Writing Password. Keep holding card to sensor till done.\n");

      byte  buffer[] = REGULARPASS;
      for (byte i = PASSLEN; i < 16; i++) buffer[i] = ' ';

      block = 1;
      //Serial.println(F("Authenticating using key A..."));
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("PCD_Authenticate() success: "));

      // Write block
      status = mfrc522.MIFARE_Write(block, buffer, 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("MIFARE_Write() success: "));

      lcd.clear();
      lcd.write("Programming Mode");
      lcd.setCursor(0, 1);
      lcd.write("Programmed");
      delay(2000);
      lcd.clear();
      lcd.write("Programming Mode");
    }
    else {
      Serial.print("FOB Access Denied\n");
      Serial.println();
      
      digitalWrite(LED_RED, HIGH);
      lcd.clear();
      lcd.write("Access Denied");
      delay(2000);
      lcd.clear();
      digitalWrite(LED_RED, LOW);

      if (byte_comp(prevUID, mfrc522.uid.uidByte, mfrc522.uid.size)) {
        tagCount++;
      }
      
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        prevUID[i] = mfrc522.uid.uidByte[i];
      }

      if (tagCount == 2) {
        Serial.println("3 Failed attempt. Locking for 30 seconds");
        
        digitalWrite(LED_RED, HIGH);
        lcd.clear();
        lcd.write("3 Failed Attempts");
        delay(2000);
        lcd.clear();
        lcd.write("Locked for 30s!");
        delay(10000);
        lcd.clear();
        digitalWrite(LED_RED, LOW);
        lcd.write("Try Again");
        
        tagCount = 0;
        
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          prevUID[i] = 0;
        }
      }
    }
    
    delay(1000); //change value if you want to read cards faster

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  else {
    digitalWrite(LED_SYSOP, LOW);
  }
}

//*****************************************************************************************//

//This function takes in two byte arrays and compares them.
//Returns 1 if the two arrays are the same otherwise it returns 0.

int byte_comp( byte* a1, byte* a2, byte  asize) {
  int flag = 0;
  for (byte i = 0; i < asize; i++) {
    if (a1[i] == a2[i])
      flag = 1;
    else {
      flag = 0;
      break;
    }
  }
  return flag;
}

// This function reads the password stored inside the card presented at the reader
// Compares it with the given password and returns 1 if matches otherwise returns 0

int checkPass(char* INPass) {
  byte buffer2[18];
  block = 1;
  len = 18;
  //Authenticate the card read
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Read the data/password stored in the card memory
  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // copy the byte data into a string for comparison
  char pass[8] = "";
  for (uint8_t i = 0; i < PASSLEN; i++) {
    pass[i] = buffer2[i] ;
  }

  if (byte_comp(INPass, pass, PASSLEN)) {
    return 1;
    //data read matches the password
  }
  else {
    return 0;
    // data read does not match the password
  }
}

// This function is used to display the UID of the RFID Card Read

void displayUID(byte* inputUID, byte sizeofUID) {
  // TAG UID information is extracted and displayed in Serial Monitor in the following segment of code.
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < sizeofUID; i++) {
    if (inputUID[i] < 0x10)
      Serial.print(F(" 0"));
    else
      Serial.print(F(" "));
    Serial.print(inputUID[i], HEX);
  }
  Serial.println();
}

// THis function reads input from the Keypad and puts the systems into different modes 
//        - Access Granted, Access Denied, Programming Mode

void keypadCheck() {
  char keyMasterPass[5] = MASTERKEYPASS;

  e = customKeypad.read();

  if (e.bit.EVENT == KEY_JUST_PRESSED) {
    if (((char)e.bit.KEY) == '#') {
      i = 0;
      Serial.print("Password Entered : ");
      
      for (uint8_t j = 0; j < 5; j++) {
        Serial.write(password[j] );
      }
      Serial.println();

      if (byte_comp(password, keyMasterPass, 5)) {
        Serial.print("Master password Detected\n");
        
        lcd.clear();
        lcd.write("Master PIN");
        lcd.setCursor(0, 1);
        lcd.write("Detected..");
        delay(2000);
        lcd.clear();
        
        padCount = 0;
        keyMasterFlag = !keyMasterFlag;
        
        if (!keyMasterFlag) {
          Serial.print("Exit Programming Mode\n");
          Serial.println();
          
          lcd.clear();
          lcd.write("Exit");
          delay(2000);
          lcd.clear();
          digitalWrite(LED_BLUE, LOW);
        }
        else {
          Serial.print("Entering Programming Mode\n");
          Serial.println();
          
          digitalWrite(LED_BLUE, HIGH);
          lcd.clear();
          lcd.write("Programming Mode");
        }
      }
      else if (byte_comp(password, keyPassword, 5) && !keyMasterFlag) {
        Serial.print("Keypas Access Granted\n");
        Serial.println();
        
        digitalWrite(LED_GREEN, HIGH);
        lcd.clear();
        lcd.write("Access Granted");
        delay(2000);
        lcd.clear();
        digitalWrite(LED_GREEN, LOW);
        
        padCount = 0;
      }
      else if (keyMasterFlag) {
        
        for (int i = 0; i < 5; i++) {
          keyPassword[i] = password[i];
        }
        
        Serial.print("Password Programmed In\n");
        Serial.println();
        
        lcd.clear();
        lcd.write("Programming Mode");
        lcd.setCursor(0, 1);
        lcd.write("Programmed");
        delay(2000);
        lcd.clear();
        lcd.write("Programming Mode");
      }
      else {
        Serial.print("Keypad Access Denied\n");
        Serial.println();
        
        digitalWrite(LED_RED, HIGH);
        lcd.clear();
        lcd.write("Access Denied");
        delay(2000);
        lcd.clear();
        digitalWrite(LED_RED, LOW);
        
        padCount++;
        if (padCount == 3) {
          Serial.println("3 Failed attempt. Locking for 30 seconds");
          
          digitalWrite(LED_RED, HIGH);
          lcd.clear();
          lcd.write("3 Failed Attempts");
          delay(2000);
          lcd.clear();
          lcd.write("Locked for 30s!");
          delay(10000);
          lcd.clear();
          digitalWrite(LED_RED, LOW);
          lcd.write("Try Again");
          
          padCount = 0;
        }
      }
      
      for (int i = 0; i < 5; i++) {
        password[i] = " ";
      }

      return;
    }
    else {
      password[i] = (char)e.bit.KEY;
      i++;
    }
  }

}


// This function runs to simulate the boot up process of our system.
void bootUP() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_SYSOP, HIGH);
  
  lcd.clear();
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING.");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING..");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING...");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING....");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING.....");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING......");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING.......");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING........");
  delay(500);
  lcd.clear();
  
  lcd.write("SMART GATEKEEPER");
  lcd.setCursor(0, 1);
  lcd.write("BOOTING.........");
  delay(500);
  lcd.clear();
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_SYSOP, LOW);
}
