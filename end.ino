#include <Key.h>
#include <Keypad.h> //keypad

#include <LiquidCrystal_I2C.h> //LCD

#include <Adafruit_NeoPixel.h> //LED

#include <SPI.h> //SPI 통신(RFID)
#include <MFRC522.h> //RFID

#define PIN A1 //LED 핀 설정
#define count_of_led 11 //led 소자 개수

#define SS_PIN 10 
#define RST_PIN 9 //RFID 관련 핀 설정

MFRC522 rfid(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key; //rfid 세팅

const int ROW_NUM    = 4; // 키패드 4열
const int COLUMN_NUM = 4; // 키패드 4행

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} //키패드 키 배치
};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(count_of_led, PIN, NEO_GRB + NEO_KHZ800); //개수,핀,품명 

byte nuidPICC[4];
byte pin_rows[ROW_NUM] = {9, 8, 7, 6};      // 가로방향 키패드 핀 설정
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2}; // 세로방향 키패드 핀 설정

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
LiquidCrystal_I2C lcd(0x3F, 16, 2); //LCD 16열, 2행

int cursorColumn = 0;

int ledPin = 13;                // choose the pin for the LED
int inputPin = A0;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status

int Home1 = 0; // (Numpad)비밀번호 맞으면1로 변경
int Home2 = 0; // (RFID)카드키 일치하면 1로 변경

void setup() {
  Serial.begin(9600); //시리얼모니터 ON
  
  pinMode(ledPin, OUTPUT);      // 13번 led
  pinMode(inputPin, INPUT);     // A0, 문 근접센서

  strip.begin(); //네오픽셀을 초기화하기 위해 모든LED를 off시킨다
  strip.show();
  
  SPI.begin(); //RFID 모듈 SPI통신 ON
  rfid.PCD_Init(); // RFID 모듈 작동시작

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF; //RFID 모듈 초기화
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Password : "); //LCD 설정 후 초기값 출력
}

String checkPassword = "1111"; //설정 pswd
String password = ""; //입력값 pswd

void loop() {
  KEY();
}

void KEY() { // LCD + 키패드
  char key = keypad.getKey();

  if (key) {
    lcd.setCursor(11 + cursorColumn, 0); //lcd에 커서이동
    lcd.print(key);                 //입력받은 키값 lcd 출력
    Serial.println(key);
    password += key; //pswd 변수에 입력키값저장
    cursorColumn++; //커서 한칸 이동
    
    if (cursorColumn == 4) {       //4자리 입력후 판단
      delay(500);
      Serial.print(password);
      Serial.print("/");
      Serial.print(checkPassword);
      Serial.println();
      
      if (password == checkPassword) { //비밀번호가 설정값과 같으면
        Home = 1; //home 1로 설정
        lcd.setCursor(0, 1);
        lcd.print("Success"); //lcd 성공 출력
      } 
      
      else {
        lcd.setCursor(0, 1);
        lcd.print("Wrong"); //lcd 실패 출력
        delay(1000);
        password = ""; //pswd 변수 초기화
        lcd.clear(); //lcd 초기화
        lcd.setCursor(0, 0);
        lcd.print("Password : ");
        cursorColumn = 0;
      } 
    }
  }
  PICC(); //RFID 부분

  // pir : 모션캡쳐

  val = digitalRead(inputPin);  //문 센서 값  val 저장
  
  /* -------필요없는 부분으로 추정---------
  if (val == 1) {            //문이 닫혀있으면


  }
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");

      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } 
  else { //문이 열려있으면
    if (pirState == HIGH) {
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
  ---------------------------------------*/

  if (val == 1) { //문이 열렸을때
    if (Home1 == 1 && Home2 == 1) { //비밀번호와 RFID가 모두 일치할때
      colorWipe(strip.Color(20, 20, 20), 5); // 주거자 ->led 순차점멸 딜레이 5
      Serial.println("HouseMaster enter the house."); //주거자 집 입장
    }
    
    else if (Home1 == 0 || Home2 == 0) { //비밀번호와 RFID중 하나라도 불일치할때
      for (int i = 0; i < count_of_led; i++)
      {
        strip.setPixelColor(i, 30, 0, 0);  // (A,R,G,B) A번째 LED를 RGB (0~255) 만큼의 밝기로 켭니다. 침입자
        Serial.println("Theif enter the house!!!!"); //외부인 집 입장
      }
      strip.show();
      delay(1000);  // 깜빡 깜빡 (1초 대기)
    }
  }
}

void PICC() { // RFID
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected.")); //새로운 카드가 태깅되면

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i]; //입력받은 카드정보 nuidPICC에 저장
    }
  else Serial.println(F("Card read previously.")); //이전카드와 같은경우 출력
  
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: ")); //16진수로는
    printHex(rfid.uid.uidByte, rfid.uid.size); //16진수 출력
    Serial.println();
    
    Serial.print(F("In dec: ")); //10진수로는
    printDec(rfid.uid.uidByte, rfid.uid.size); //10진수 출력
    Serial.println();
    
    Serial.println(String(nuidPICC[1]));
    
    if (String(nuidPICC[1]) == "37" ) { //만약 카드입력값 1번이 37이라면
      Serial.print("Card Secure Unlocked"); //수정부분 
      Home2 = 1;
    }


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void colorWipe(uint32_t c, uint8_t wait) { //led 순차점멸
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void printHex(byte *buffer, byte bufferSize) { //카드정보값 16진수로 출력
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) { //카드정보값 10진수로 출력
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}