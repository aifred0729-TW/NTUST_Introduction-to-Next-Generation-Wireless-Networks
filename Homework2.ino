#include <SPI.h>
#include "MFRC522.h"

#define SS_PIN 10
#define RST_PIN 9
#define MAX_SIZE 0xf

MFRC522 rfid(SS_PIN, RST_PIN);

bool isAdmin = false;
int adminID = 0;
int pinBase = 3;

struct Database {
  int id;
  String CID;
  String KEY;
};

Database db[0xf] = {
  {0, "D37EC72E", "000000"}
};

int dbCount = 1;

void setup() {

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  Serial.println();
  Serial.println("===========================================");
  Serial.println("[+] Initialization Info");
  Serial.println("[-]     - Default CID : " + db[0].CID);
  Serial.println("[-]     - Default KEY : " + db[0].KEY);
  Serial.print("[-]     - MFRC522 version: ");
  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.println(v, HEX);

  for (int i = 0; i < 3; i++) {
    pinMode(pinBase + i, OUTPUT);
  }

}

void callOUTPUT(int PIN) {
  digitalWrite(PIN, HIGH);
  delay(300);
  digitalWrite(PIN, LOW);
  delay(300);

  return;
}

String readRFID() {

  String rfidID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    rfidID += String(rfid.uid.uidByte[i], HEX);
  }
  rfidID.toUpperCase();
  Serial.println("Read RFID ID: " + rfidID);
  rfid.PICC_HaltA();

  return rfidID;
}

int findIndex(String cid) {
  for (int i = 0; i < dbCount; i++) {
    if (db[i].CID == cid)
      return i;
  }
  return -1;
}

String readREQ() {
    while (Serial.available() == 0);
    String req = Serial.readStringUntil('\n');
    req.trim();
    return req;
}

bool authentication(String cid) {
  int index = findIndex(cid);
  if (index == -1) return false;

  Serial.println("[+] KEY Required, Please Enter the KEY.");
  if (db[index].KEY != readREQ()) return false;
  if (db[index].id == adminID) isAdmin = true;

  return true;
}

void errorOUTPUT() {
  callOUTPUT(5);
  callOUTPUT(5);
  callOUTPUT(4);
  callOUTPUT(4);

  return;
}

void successOUTPUT() {
  callOUTPUT(3);
  callOUTPUT(4);

  return;
}

String getArgument(String command, int index) {
  command.trim();
  int start = 0;
  int end = -1;
  int found = 0;

  for (int i = 0; i <= command.length(); i++) {
    if (i == command.length() || command.charAt(i) == ' ') {
      found++;
      if (found == index + 1) {
        end = i;
        return command.substring(start, end);
      }
      while (i < command.length() && command.charAt(i) == ' ') i++;
      start = i;
      i--;
    }
  }

  return "";
}

int getOPCODE(String command) {
  String cmd = getArgument(command, 0);
  if (cmd == "UPDATE") return 1;
  if (cmd == "DEL") return 2;
  if (isAdmin) {
    if (cmd == "ADD") return 3;
    if (cmd == "LIST") return 4;
    if (cmd == "REMOVE") return 5;
    if (cmd == "EDITADMIN") return 6;
  }
  return 0;
}

void UPDATE(int index) {

  String password1 = "";
  String password2 = "";
  
  while (true) {
    
    Serial.println("[+] Enter New Password : ");
    password1 = readREQ();
    
    Serial.println("[+] Enter New Password Again : ");
    password2 = readREQ();

    if (password1 == password2) {
      db[index].KEY = password1;
      Serial.println("[+] Password Changed.");
      return;
    }

    Serial.println("[!] Password does not match, try again.");

  }
}

void DEL(int index) {
  for (int i = index; i < dbCount - 1; i++) db[i] = db[i+1];
  dbCount--;
  Serial.println("[+] User Deleted.");
  return;
}

void ADD() {
  if (dbCount >= MAX_SIZE) {
    Serial.println("[!] Database full.");
    return;
  }

  Serial.println("[+] Please tap a new card to add.");

  String newCID = "";

  while (true) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) newCID = readRFID();
    else continue;

    if (findIndex(newCID) != -1) {
      Serial.println("[!] Card already exists in database.");
      return;
    }

    Serial.println("[+] Enter password for this card:");
    String newKEY = readREQ();

    db[dbCount].id = dbCount;
    db[dbCount].CID = newCID;
    db[dbCount].KEY = newKEY;
    dbCount++;

    Serial.println("[+] New card added successfully!");
    break;
  }

  return;
}

void LIST() {
  Serial.println("===========================================");
  Serial.println(" ID  |    CID     |   KEY     |  ROLE");
  Serial.println("-------------------------------------------");
  for (int i = 0; i < dbCount; i++) {
    Serial.print(" 0x");
    Serial.print(i + 1);
    Serial.print("   ");
    Serial.print(db[i].CID);
    Serial.print("   ");
    Serial.print(db[i].KEY);
    Serial.print("   ");
    if (i == adminID) Serial.println("admin");
    else Serial.println("member");
  }

  return;
}

void REMOVE(String command) {
  int target = getArgument(command, 1).toInt() - 1;
  if (target < 0 || target >= dbCount) {
    Serial.println("[!] Invalid index.");
    return;
  }
  if (target == adminID) {
    Serial.println("[!] Cannot remove admin account.");
    return;
  }

  for (int i = target; i < dbCount - 1; i++) db[i] = db[i + 1];
  dbCount--;

  Serial.println("[+] User removed successfully.");
}

void EDITADMIN(String command) {
  int target = getArgument(command, 1).toInt() - 1;
  if (target < 0 || target >= dbCount) {
    Serial.println("[!] Invalid index.");
    return;
  }
  adminID = target;
  Serial.print("[+] Admin changed to user index.");
  Serial.println(target + 1);

  return;
}

void kernel(String cid) {
  Serial.println("===========================================");
  Serial.println("[+] Welcome to Unknown System v1.0");
  Serial.print("[-]     - Current UserID : ");
  Serial.println((findIndex(cid)));
  Serial.print("[-]     - Current Permission : ");

  if (isAdmin) Serial.println("Administrator");
  else Serial.println("User");

  Serial.println("[-]     - Operations");
  Serial.println("[-]     --> UPDATE / DEL");

  if (isAdmin) {
    Serial.println("[-]     --> ADD / LIST / REMOVE / EDITADMIN");
  }

  String command = readREQ();

  switch (getOPCODE(command)) {
    case 1:
      Serial.println("[+] UPDATE");
      UPDATE(findIndex(cid));
      break;
    case 2:
      Serial.println("[+] DEL");
      DEL(findIndex(cid));
      break;
    case 3:
      Serial.println("[+] ADD");
      ADD();
      break;
    case 4:
      Serial.println("[+] LIST");
      LIST();
      break;
    case 5:
      Serial.println("[+] REMOVE");
      REMOVE(command);
      break;
    case 6:
      Serial.println("[+] EDITADMIN");
      EDITADMIN(command);
      break;
    default:
      Serial.println("[!] Unknown OPCODE");
      break;
  }

} 

void loop() {

  int index = 0;
  String cid = "";

  Serial.println("===========================================");
  Serial.println("Waiting for card approching...");

  while (true) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) cid = readRFID();
    else continue;

    if (!authentication(cid)) {

      Serial.println("[!] Wrong CID or KEY");
      errorOUTPUT();
      delay(1000);
      
      Serial.println("===========================================");
      Serial.println("Waiting for card approching...");
      continue;
    }

    successOUTPUT();

    kernel(cid);
    isAdmin = false;

    Serial.println("===========================================");
    Serial.println("Waiting for card approaching...");
  }

}