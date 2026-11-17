#include <Keyboard.h>
#include <Adafruit_MPR121.h>
#include <Wire.h>
#include "src/PN532/PN532_I2C.h"
#include "src/PN532/PN532.h"

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
bool cardPresent = false;

/* Software Debounce Interval */
#define DEBOUNCE 10
unsigned int buttonPin[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
unsigned long keyTimer[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char asciiKey[12] = {0x39, 0x36, 0x33, 0x63, 0x38, 0x35, 0x32, 0x64, 0x37, 0x34, 0x31, 0x30};  // 7894561230cd  en clavier US

/* DO NOT MESS WITH THE LINES BELOW UNLESS YOU KNOW WHAT YOU'RE DOING */

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

#include "src/Cardio.h"
  Cardio_ Cardio;
 
void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");

  Keyboard.begin();
  nfc.begin();

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A))
  {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
    
  Serial.println("Waiting for an ISO14443A card");

//  Cardio.begin(false);
}

unsigned long lastReport = 0;
uint16_t cardBusy = 0;

void loop() {
  checkAllKeyEvents2();  // fonction simplifée de pavenum… bug souvent en répétant le caractére cccccccccc
  // si ça se produit c'est la sensibilité des touches qui est en cause, voir pour modifier les deux threshold de la lib MPR121
  //  pavenum();
  readcards(); // Met à jour l'état de la carte (cardPresent = true ou false)

  if (cardPresent) {
    checkAllKeyEvents2(); // Le clavier ne fonctionne QUE si une carte est active
  } else {
    // Optionnel de sécurité : s'assurer que les variables du tactile se réinitialisent
    lasttouched = cap.touched(); 
  }
}

void checkAllKeyEvents2() {
  currtouched = cap.touched();
  
  for (uint8_t i = 0; i < 12; i++) {
    // Si la touche est pressée maintenant MAIS ne l'était pas avant (Front montant)
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i))) {
      Keyboard.press(asciiKey[i]);
      delay(10); // Petit délai anti-rebond matériel optionnel
    }
    // Si la touche n'est plus pressée MAIS l'était avant (Front descendant)
    else if (!(currtouched & _BV(i)) && (lasttouched & _BV(i))) {
      Keyboard.release(asciiKey[i]);
    }
  }
  
  lasttouched = currtouched;
}

void readcards() {
  /* NFC */
  if (millis()-lastReport < cardBusy) return;
//  
  cardBusy = 0;
  uint8_t uid[8] = {0,0,0,0,0,0,0,0};
  uint8_t hid_data[8] = {0,0,0,0,0,0,0,0};
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)


  // check for FeliCa card
  uint8_t ret;
  uint16_t systemCode = 0xFFFF;
  uint8_t requestCode = 0x01;       // System Code request
  uint8_t idm[8];
  uint8_t pmm[8];
  uint16_t systemCodeResponse;

  // Wait for an FeliCa type cards.
  // When one is found, some basic information such as IDm, PMm, and System Code are retrieved.
  ret = nfc.felica_Polling(systemCode, requestCode, idm, pmm, &systemCodeResponse, 40); // à changer pour détecter +/- vite ce type de carte (ori = 500)

    if (ret == 1) {

/*   Found a FeliCa card!
     IDm:  01 2E 5C E2 71 4B 7E AC
     PMm:  00 F1 00 00 00 01 43 00
     System Code: 88B4               */
      
      Serial.println("Found a FeliCa card!");
      Serial.print("  IDm: ");
      nfc.PrintHex(idm, 8);
      Serial.print("  PMm: ");
      nfc.PrintHex(pmm, 8);
      Serial.print("  System Code: ");
      Serial.print(systemCodeResponse, HEX);
      Serial.print("\n");
      Serial.println(""); 
      Cardio.setUID(2, idm);
      Cardio.sendState();      
      lastReport = millis();
      cardBusy = 3000;
      cardPresent = true; // <--- Carte présente
      uidLength = 0;
      return;
    }

   // check for ISO14443 card

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength))
  {
/*    Found a ISO14443 card!
      UID Length: 4 bytes
      UID Value:  0x33 0x45 0x89 0xC 0x0 0x0 0x0 0x0      */
    
    Serial.println("Found a ISO14443 card!");
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");  
    for (int i=0; i<8; i++) {
      hid_data[i] = uid[i%uidLength];
      Serial.print(" 0x");Serial.print(uid[i], HEX); 
    }
    Serial.println(""); 
    Serial.println(""); 
    Cardio.setUID(1, hid_data);
    Cardio.sendState();

    lastReport = millis();
    cardBusy = 5000;
    cardPresent = true; // <--- Carte présente
    return;
  }
  // Aucune carte détectée
  // Si on veut désactiver immédiatement les touches quand la carte s'éloigne :
  cardPresent = false; 
  // Optionnel : s'assurer qu'on relâche tout si la carte part
  Keyboard.releaseAll();
  
  lastReport = millis();
  cardBusy = 200;

// delay(2000);
}


void pavenum() {
  //https://en.wikipedia.org/wiki/ASCII
  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) )
    {
      Serial.print(i); Serial.println(" touched | ");
      
      switch (i)
      {
        case 0:          // // ASCII pour 9 anglais, when var equals 0
          Keyboard.press(0x39);
          Keyboard.releaseAll();  
          break;
        case 1:          // // ASCII pour 6 anglais, when var equals 1
          Keyboard.press(0x36);
          Keyboard.releaseAll();  
          break;
        case 2:          // // ASCII pour 3 anglais, when var equals 2
          Keyboard.press(0x33);
          Keyboard.releaseAll();  
          break;
        case 3:
//          Keyboard.press(0x33); // rien
//          Keyboard.releaseAll();
          break;
        case 4:
          Keyboard.press(0x38); // ASCII pour 8 anglais
          Keyboard.releaseAll();
          break;
        case 5:
          Keyboard.press(0x35); // ASCII pour 5 anglais
          Keyboard.releaseAll();
          break;
        case 6:
          Keyboard.press(0x32); // ASCII pour 2 anglais
          Keyboard.releaseAll();
          break;
        case 7:
          Keyboard.press(0x30); // ASCII pour 00 anglais
          Keyboard.releaseAll();
          break;
        case 8:
          Keyboard.press(0x37); // ASCII pour 7 anglais
          Keyboard.releaseAll();
          break;
        case 9:
          Keyboard.press(0x34); // ASCII pour 4 anglais
          Keyboard.releaseAll();
          break;
         case 10:
          Keyboard.press(0x31); // ASCII pour 1 anglais
          Keyboard.releaseAll();
          break;         
        case 11:
          Keyboard.press(0x30); // ASCII pour 0 anglais
          Keyboard.releaseAll();
          break;          
        default:
          // if nothing else matches, do the default
          // default is optional
          break;
      }
    }
  }
  // reset our state
  lasttouched = currtouched;

 //delay(100);
}
