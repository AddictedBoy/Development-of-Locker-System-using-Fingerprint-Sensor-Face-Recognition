#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include "string.h"

#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3);   //serial to fingerprint sensor
SoftwareSerial SoftSerial(4, 5); //serial to esp32 cam

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//variables to keep track of various tasks
uint8_t id;
uint8_t number;
uint8_t number_delete;
uint8_t user_delete;
uint8_t finger_face;
uint8_t i;
uint8_t enroll_ok;
uint8_t both;
uint8_t es_finger;
uint8_t es_face;
uint8_t choose;
uint8_t ok_search;
uint8_t mismatch;

//variable for serial communication
bool done;

//variables to keep track of user using either fingerprint or face recognition
uint8_t user1 = 0;
uint8_t user2 = 0;
uint8_t user_pref;

//variables to keep track of locker availability
int facelist1 = 0;
int facelist2 = 0;

//servo variables
Servo myServo1;
Servo myServo2;
int pos = 0;

//magnetic reed switch and button variables
int switch1 = 6;
int switch2 = 7;
int buttonApin = 10;
int buttonBpin = 11;
int buttonCpin = 12;

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SoftSerial.begin(115200);
  SoftSerial.write("");

  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("UNO System!");

  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);
  pinMode(buttonApin, INPUT_PULLUP);
  pinMode(buttonBpin, INPUT_PULLUP);
  pinMode(buttonCpin, INPUT_PULLUP);
  myServo1.attach(8);
  myServo2.attach(9);
  delay(100);
  myServo1.write(0);
  myServo2.write(0);

  finger.begin(57600);

  // find fingerprint sensor
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }
}

//function to read inputs from serial monitor
uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {
  //initialise and reset everytime loop comes here
  String terminalText;
  id = 0;
  finger_face = 0;
  user_pref = 0;
  user_delete = 0;
  number_delete = 0;
  choose = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1 new user");
  lcd.setCursor(0, 1);
  lcd.print("2 old user");
  //Serial.println(F("Select: 1 for new user (enroll) and 2 for old user (searching)"));

  while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH) and (digitalRead(buttonCpin) == HIGH));
  if (digitalRead(buttonApin) == LOW) {
    number = 1;
  }
  else if (digitalRead(buttonBpin) == LOW) {
    number = 2;
  }
  else if (digitalRead(buttonCpin) == LOW) {
    number = 3;
  }
  delay(1000);
  switch (number) {
    case 1:
      //use either fingerprint/face recognition or both
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Use 1 or both?");
      while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
      if (digitalRead(buttonApin) == LOW) {
        both = 1;
      }
      else if (digitalRead(buttonBpin) == LOW) {
        both = 2;
        finger_face = 3;
      }

      delay(1000);

      //if user choose either 1, choose either fingerprint or face
      if (finger_face != 3) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("1 fingerprint");
        lcd.setCursor(0, 1);
        lcd.print("2 face");
        //Serial.println(F("Select: 1 for fingerprint sensor and 2 for face recognition system"));

        while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
        if (digitalRead(buttonApin) == LOW) {
          finger_face = 1;
        }
        else if (digitalRead(buttonBpin) == LOW) {
          finger_face = 2;
        }
      }
      delay(1000);

      //enroll function
      Serial.println(F("Selected enrolling"));
      if (finger_face == 1) {
        Serial.println(F("Ready to enroll a fingerprint!"));
      }
      else if (finger_face == 2) {
        Serial.println(F("Ready to enroll a face!"));
      }

      //select locker 1 or 2
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("locker 1 or 2?");
      while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
      if (digitalRead(buttonApin) == LOW) {
        Serial.println(F("Selected locker 1"));
        id = 1;
        if (user1 != 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Locker 1 in use!");
          finger_face = 0;
        }
      }
      else if (digitalRead(buttonBpin) == LOW) {
        id = 2;
        if (user2 != 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Locker 2 in use!");
          finger_face = 0;
        }
      }
      delay(1000);

      //start to enroll
      if (finger_face == 1) {
        enroll_ok = finger_enroll(id);

        if (enroll_ok == 1) {
          locker(id, 1);
        }
        delay(50);
        enroll_ok = 0;
      }
      else if (finger_face == 2) {
        enroll_ok = face_enroll(id);
        if (enroll_ok == 1) {
          locker(id, 2);
        }
      }
      else if (finger_face == 3) {
        es_finger = finger_enroll(id);
        es_face = face_enroll(id);
        if (es_finger == es_face) {
          locker(id, 3);
        }

      }
      break;
    case 2:
      //choose either locker 1 or 2
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("locker 1 or 2");
      //Serial.println(F("Select : 1 for locker 1 and 2 for locker 2"));
      //number = readnumber();

      while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
      if (digitalRead(buttonApin) == LOW) {
        number = 1;
      }
      else if (digitalRead(buttonBpin) == LOW) {
        number = 2;
      }

      //retrieve int variable for fingeprint/face from finger_face
      if (number == 1) {
        user_pref = user1;

      }
      else if (number == 2) {
        user_pref = user2;
      }
      delay(1000);

      //if user don't want to continue using, delete data after searching is successful
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Continue using locker?");

      while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
      if (digitalRead(buttonApin) == LOW) {
        number_delete = 1;
      }
      else if (digitalRead(buttonBpin) == LOW) {
        number_delete = 2;
      }
      delay(1000);

      //retrieve int variable for fingeprint/face from finger_face
      if (number_delete == 1) {
        if (number == 1) {
          user_delete = 1; //don't delete for user 1
        }
        else if (number == 2) {
          user_delete = 2; //don't delete for user 2
        }
      }
      else if (number_delete == 2) {
        if (number == 1) {
          user_delete = 3; //delete for user 1
        }
        else if (number == 2) {
          user_delete = 4; //delete for user 2
        }
      }

      //start search function
      if (user_pref == 3) {
        if (number_delete != 2) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("fingerprint or face");
          while ((digitalRead(buttonApin) == HIGH) and (digitalRead(buttonBpin) == HIGH));
          if (digitalRead(buttonApin) == LOW) {
            choose = 1;
          }
          else if (digitalRead(buttonBpin) == LOW) {
            choose = 2;
          }
        }
      }

      if (user_pref == 1) {
        ok_search = finger_search(user_pref, user_delete, 0);
        delay(1000);
      }
      else if (user_pref == 2) {
        ok_search = face_search(user_pref, user_delete);
      }
      else if (user_pref == 3) {
        if (choose == 1) {
          es_finger = finger_search(user_pref, user_delete, 0);
        }
        else if (choose == 2) {
          es_face = face_search(user_pref, user_delete);
        }
        else {
          es_finger = finger_search(user_pref, user_delete, 1);
          es_face = face_search(user_pref, user_delete);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("All deleted!");
        }
      }

      if (user_pref == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Locker not registered!");
      }
      delay(1000);
      break;
    case 3:
      //debugging function
      SoftSerial.listen();

      SoftSerial.write("zzz");
      done = false;
      while (!done) {
        Serial.println(F("Waiting"));
        delay(50);
        if (SoftSerial.available()) {
          terminalText = SoftSerial.readStringUntil('\n');
          Serial.println(F("SoftSerial in"));
          Serial.println(terminalText);
          if (terminalText.indexOf("y") != -1) { //True
            Serial.println("Image is True");
            done = true;
          }
          else {
            done = true;
          }
          delay(500);
          terminalText = SoftSerial.readStringUntil('\n');
        }
      }
      break;
    default:
      Serial.println("Error!");
      delay(50);
      break;
  }

}

//locker function to open and close locker
void locker(int id, int auth) {
  //open locker
  if (id == 1) {
    myServo1.write(180);
    delay(50);
  }
  else if (id == 2) {
    myServo2.write(180);
    delay(50);
  }

  //close locker
  if (id == 1) {
    while (digitalRead(switch1) == HIGH);
    //if switch1 is low, close locker
    myServo1.write(0);
    if (auth == 1) {
      user1 = 1;
    }
    else if (auth == 2) {
      user1 = 2;
    }
    else if (auth == 3) {
      user1 = 3;
    }
  }
  else if (id == 2) {
    while (digitalRead(switch2) == HIGH);
    //if switch2 is low, close locker
    myServo2.write(0);
    if (auth == 1) {
      user2 = 1;
    }
    else if (auth == 2) {
      user2 = 2;
    }
    else if (auth == 3) {
      user2 = 3;
    }
  }
  delay(1000);
}

//enroll fingerprint function
int finger_enroll(uint8_t id) {
  int finger_ok;
  mySerial.listen();
  
  delay(50);
  for (i = 0; i <= 2; i++) {
    getFingerprintEnroll();
    delay(500);
    if (mismatch == 1) {
      enroll_ok = 0;
      mismatch = 0;
    }
    if (enroll_ok == 1) {
      //get out of for loop early if fingerprint is registered
      finger_ok = 1;
      break;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Try again!");
  }
  return finger_ok;
}

//enroll face function
int face_enroll(int id) {
  int face_ok;
  String terminalText;
  SoftSerial.listen();

  //face recognition system
  if (id == 1) {
    SoftSerial.write("aaa");
  }
  else if (id == 2) {
    SoftSerial.write("ccc");
  }
  delay(5000);

  done = false;
  while (!done) {
    delay(50);
    if (SoftSerial.available()) {
      terminalText = SoftSerial.readStringUntil('\n');
      delay(1000);
      if ((terminalText.indexOf("b") != -1) or (terminalText.indexOf("d") != -1)) { //True
        if (terminalText.indexOf("b") != -1) {
          facelist1 = 1;
        }
        else if (terminalText.indexOf("d") != -1) {
          facelist2 = 1;
        }
        done = true;
        face_ok = 1;
      }
      else if (terminalText.indexOf("s") != -1) { //False
        done = true;
        face_ok = 0;
      }
    }
  }
  return face_ok;
}

//search fingerprint function
int finger_search(int id, int user_delete, int skip) {
  int search_ok;
  mySerial.listen();

  //if doesn't detect after 50 times, exit searching function
  for (i = 0; i <= 50; i++) {
    getFingerprintID();
    if (finger.fingerID) {
      break;
    }
  }
  if (finger.fingerID == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("fingerprint not found!");
    search_ok = 0;
  }
  delay(50);

  if (finger.fingerID == 1) {
    if ((skip != 1) && (finger.fingerID == id)) {
      locker(1, 0);
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Authentication failed!");
    }
    search_ok = 1;
    if (user_delete == 3) {
      //delete function
      deleteFingerprint(finger.fingerID);
      user1 = 0;
      delay(50);
    }
    else if (user_delete == 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have a nice day!"));
    }
  }
  else if (finger.fingerID == 2) {
    if ((skip != 1) && (finger.fingerID == id)) {
      locker(2, 0);
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Authentication failed!");
    }
    search_ok = 1;
    if (user_delete == 4) {
      //delete function
      deleteFingerprint(finger.fingerID);
      delay(50);
      user2 = 0;
    }
    else if (user_delete == 2) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have a nice day!"));
    }
  }
  return search_ok;
}

//search face function
int face_search(int id, int user_delete) {
  int search_ok;
  String terminalText;
  SoftSerial.listen();

  if (user_delete == 1) {
    SoftSerial.write("eee");
  }
  else if (user_delete == 2) {
    SoftSerial.write("ggg");
  }
  else if (user_delete == 3) {
    SoftSerial.write("iii");
  }
  else if (user_delete == 4) {
    SoftSerial.write("kkk");
  }

  delay(5000);

  done = false;
  while (!done) {
    delay(50);
    if (SoftSerial.available()) {
      terminalText = SoftSerial.readStringUntil('\n');
      delay(1000);
      if ((terminalText.indexOf("f") != -1) or (terminalText.indexOf("h") != -1)) { //True
        done = true;
        locker(number, 0);
        search_ok = 1;
      }
      else if ((terminalText.indexOf("j") != -1) or (terminalText.indexOf("l") != -1)) { //True
        done = true;
        if (terminalText.indexOf("j") != -1) {
          facelist1 = 0;
          user1 = 0;
        }
        else if (terminalText.indexOf("l") != -1) {
          facelist2 = 0;
          user2 = 0;
        }
        locker(number, 0);
        search_ok = 1;

      }
      else if ((terminalText.indexOf("m") != -1) or (terminalText.indexOf("n") != -1) or (terminalText.indexOf("o") != -1) or (terminalText.indexOf("p") != -1)) { //False
        done = true;
        search_ok = 0;
      }
    }
  }
  return search_ok;
}



//Adafruit function
uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  delay(500);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    mismatch = 1;
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    enroll_ok = 1;
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

//Adafruit function
uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  return p;
}

//Adafruit function
uint8_t getFingerprintID() {
  //reset fingerID to 0 if this function has been used before
  finger.fingerID = 0;
  //keep looping function until ID has been found
  while (1) {
    uint8_t p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }

    // OK success!

    p = finger.image2Tz();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
      //return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
      //return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
      //return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
      //return p;
      default:
        Serial.println("Unknown error");
        //return p;
    }

    // OK converted!
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
      Serial.println("Found a print match!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      //return p;
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println("Did not find a match");
      //return p;
    } else {
      Serial.println("Unknown error");
      //return p;
    }

    // found a match!
    if (finger.fingerID != 0) {
      Serial.print("Found ID #"); Serial.print(finger.fingerID);
      Serial.print(" with confidence of "); Serial.println(finger.confidence);

      return finger.fingerID;
    }
  }
}
