#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

#define Unlock_LED 1
#define Unlock_Sound 0

#define RST_PIN A0  // 讀卡機的重置腳位
#define SS_PIN 10 // 晶片選擇腳位

#define KEY_ROWS 4 // 按鍵模組的列數
#define KEY_COLS 4 // 按鍵模組的行數

#define PassWord_Size 17
#define Delay_Time 2000
#define RFID_Size 4

// 依照行、列排列的按鍵字元（二維陣列）
char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte colPins[KEY_COLS] = { 5, 4, 3, 2 };  // 按鍵模組，行1~4接腳。
byte rowPins[KEY_ROWS] = { 9, 8, 7, 6 };  // 按鍵模組，列1~4接腳。

LiquidCrystal_I2C lcd(0x27, 16, 2); //設定LCD位置0x27,設定LCD大小為16*2
MFRC522 mfrc522(SS_PIN, RST_PIN);  // 建立MFRC522物件

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

byte Correct_RFID[] = {242, 166, 180, 115};
byte *Correct_RFIDptr = Correct_RFID;

short int PassWord_Counter = 0;
char PassWord[PassWord_Size] = {};  //使用者輸入的密碼
char Correct_PassWord[PassWord_Size] = { "0123456789" }; //正確密碼
char *Correct_PassWordptr = Correct_PassWord;

void Orginal_Set() {  //初始設定
  lcd.setCursor(0, 0);    //行 , 列
  lcd.print("Enter Password:");

  lcd.setCursor(0, 1);
  lcd.cursor(); // 顯示游標
  lcd.blink(); // 讓游標閃爍
} //void

short int lengthcounter(char *id) { //計算正確密碼的長度
  short int counter = 0;
  while (*id) {
    id++;
    counter++;
  } //while
  return counter;
}//short int

bool Proper_RFID(byte *id) {  //判斷 RFID 是否正確
  for (int i = 0; i < RFID_Size; i++) {
    if (id[i] != Correct_RFID[i]) {
      return false;
    } //if
  } //for i
  return true;
} //bool
bool Proper_PassWord() {  //判斷密碼是否正確
  for (int i = 0; i < lengthcounter(Correct_PassWordptr); i++) {
    if (PassWord[i] != Correct_PassWord[i]) {
      return false;
    } //if
  } //for
  return true;
} //bool

void RFID_Compare_Set(byte *id) {
  if (Proper_RFID(id) == 0) { //如果錯誤
    lcd.setCursor(0, 1);
    lcd.print("Wrong Key");

    digitalWrite(Unlock_Sound, HIGH);
    delay(Delay_Time);
    Enter_Set('*');
    digitalWrite(Unlock_Sound, LOW);
  } //if
  else if (Proper_RFID(id) == 1) {  //如果正確
    lcd.setCursor(0, 1);
    lcd.print("Correct Key");

    digitalWrite(Unlock_LED, HIGH);
    delay(Delay_Time);
    Enter_Set('*');
    digitalWrite(Unlock_LED, LOW);
  } //else if
}   //void
void PassWord_Compare_Set() {
  if (Proper_PassWord() == 0) { //如果錯誤
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong Password");

    digitalWrite(Unlock_Sound, HIGH);
    delay(Delay_Time);
    lcd.clear();
    Orginal_Set();
    PassWord_Counter = 0;
    digitalWrite(Unlock_Sound, LOW);
  } //if
  else if (Proper_PassWord() == 1) {  //如果正確
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Password Correct");

    digitalWrite(Unlock_LED, HIGH);
    delay(Delay_Time);
    Enter_Set('*');
    digitalWrite(Unlock_LED, LOW);
  } //else if
} //void

void Delet_Set() {  //逐個刪除密碼
  lcd.clear();
  Orginal_Set();
  PassWord[PassWord_Counter] = ' '; //刪除字元
  if (PassWord_Counter > 0) PassWord_Counter -= 1;  //防止 PassWord_Counter 小於 0 會有 BUG

  lcd.setCursor(0, 1);
  for (int i = 0; i < PassWord_Counter; i++) {
    lcd.print(PassWord[i]);
  } //for
} //void

void Enter_Set(char enter) {  //判斷是否為特殊字元
  if (enter == '*') { //clear
    lcd.clear();
    Orginal_Set();
    PassWord_Counter = 0;
  } //if
  else if (enter == 'D') {  //delet
    Delet_Set();
  } //else if
  else if (enter == '#') {   //enter
    PassWord_Compare_Set();
  } //else if
  else if (PassWord_Counter <= PassWord_Size) {  //超過 PassWord_Size 會顯示不了
    lcd.print(enter);
    PassWord[PassWord_Counter] = enter;
    PassWord_Counter++;
  } //if
  else {  //例外
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error");

    delay(Delay_Time);
    lcd.clear();
    Orginal_Set();
    PassWord_Counter = 0;
  } //else
} //void

void setup() {
  pinMode(Unlock_LED, OUTPUT);
  pinMode(Unlock_Sound, OUTPUT);
  digitalWrite(Unlock_LED, LOW);
  digitalWrite(Unlock_Sound, LOW);

  SPI.begin();
  mfrc522.PCD_Init();   // 初始化MFRC522讀卡機模組

  lcd.init();
  lcd.backlight(); //開啟背光
  Orginal_Set();  //初始化
} //void

void loop() {
  char key = myKeypad.getKey(); //取得鍵盤輸入

  if (PassWord_Counter == 0) {    //防止輸入密碼的時候又逼卡
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
      byte idSize = mfrc522.uid.size;   // 取得UID的長度

      MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RFID: ");

      lcd.setCursor(5, 0);
      for (byte i = 0; i < idSize; i++) {  // 逐一顯示UID碼
        lcd.print(id[i], HEX);  // 以16進位顯示UID值
        if (i != idSize - 1) {
          lcd.print(":");
        } //if
      } //for

      RFID_Compare_Set(id);

      mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
    } //if

    if (key) {  //接收鍵盤的數字
      Enter_Set(key);
    } //if
  }
  else if (PassWord_Counter != 0) { //如果已經有輸入數字則繼續輸入
    if (key) {
      Enter_Set(key);
    } //if
  } //else if

} //void end
