#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define BTN_LEFT   2
#define BTN_MID    3
#define BTN_RIGHT  4

int hunger = 100;
int happy = 100;
int energy = 100;
//나이추가
int age = 0;
unsigned long ageTimer = 0;

int menuIndex = 0;
unsigned long lastUpdate = 0;
unsigned long lastAnim = 0;
bool animFrame = false;

#define BUZZER 5   // 부저 핀

unsigned long hatchFlashStart = 0;
bool hatching = false;

int level = 1;      // 1=아기, 2=성장
unsigned long growTimer = 0;
//진화
int evolution = 0;
//배설
bool poop = false;
unsigned long poopTimer = 0;
//밤낮시간
unsigned long worldTimer;
bool isDay = true;

enum GameState {
  EGG,
  MENU,
  DEAD
};
GameState state = EGG;

//일정시간후 부화
unsigned long eggStartTime;

enum Mood {
  NORMAL,
  HAPPY,
  HUNGRY,
  SLEEPY
};

Mood getMood() {

  if (energy < 20) return SLEEPY;
  if (hunger < 30) return HUNGRY;
  if (happy > 80) return HAPPY;

  return NORMAL;
}

void saveGame() {
  EEPROM.write(0, hunger);
  EEPROM.write(1, happy);
  EEPROM.write(2, energy);
  EEPROM.write(3, level);
  EEPROM.commit();
}

void loadGame() {
  hunger = EEPROM.read(0);
  happy  = EEPROM.read(1);
  energy = EEPROM.read(2);
  level  = EEPROM.read(3);

  // 처음 실행 시 값이 이상하면 초기화
  if (hunger > 100 || hunger == 0) {
    hunger = 100;
    happy = 100;
    energy = 100;
    level = 1;
  }
}

//setup//
void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_MID, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  Wire.begin(8, 9);
  pinMode(BUZZER, OUTPUT);
  growTimer = millis();
  //나이타이머초기화
  ageTimer = millis();
  //밤낮시간
  worldTimer = millis();

  EEPROM.begin(10);
  //배설
  poopTimer = millis();
  //부화시간
  eggStartTime = millis();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  EEPROM.begin(10);
  loadGame();

  display.clearDisplay();
}

void loop() {
  if (state != DEAD) updateStats();
  //5초후 부화
  if (state == EGG && millis() - eggStartTime > 5000) {
    state = MENU;
  }
  //시간이후 성장.
  if (state == MENU && millis() - growTimer > 30000) {
    level = 2;   // 30초 후 성장
  }
  //나이 타이머 여기서 테스트용으로 1분을 1살로 셋팅해놨다. 다음에 조정가능
  if (millis() - ageTimer > 60000) {  // 1분 = 1살
    age++;
    ageTimer = millis();
  }
  //배설
  if (!poop && millis() - poopTimer > 45000) {  // 45초마다
    poop = true;
  }
  
  //나이에 따라 진화
  if (age >= 3 && age < 7) {
    evolution = 1;
  }
  if (age >= 7 && age < 10) {
    evolution = 2;
  }
  if (age >= 10) {
    evolution = 3;
  }

  static unsigned long lastSave = 0;
  if (millis() - lastSave > 10000) {   // 10초마다 저장
    saveGame();
    lastSave = millis();
  }
  
  //read draw는 아래 놓는게 맞는거 같아.
  readButtons();
  drawScreen();

}

void updateStats() {
  if (millis() - lastUpdate > 5000) {
    hunger -= 2;
    happy -= 1;
    energy -= 1;
    lastUpdate = millis();
  }
  if (poop) {
    happy -= 1;
  }
  if (hunger <= 0 || energy <= 0) {
    state = DEAD;
    tone(BUZZER, 400, 300);
    delay(350);
    tone(BUZZER, 200, 500);
    saveGame();
  }
}
void readButtons() {

  static bool lastLeft = HIGH;
  static bool lastRight = HIGH;
  static bool lastMid = HIGH;

  bool currentLeft = digitalRead(BTN_LEFT);
  bool currentRight = digitalRead(BTN_RIGHT);
  bool currentMid = digitalRead(BTN_MID);

  // 왼쪽 버튼 (눌렸을 때 한 번만)
  if (lastLeft == HIGH && currentLeft == LOW) {
    menuIndex--;
    if (menuIndex < 0) menuIndex = 2;
  }

  // 오른쪽 버튼
  if (lastRight == HIGH && currentRight == LOW) {
    menuIndex++;
    if (menuIndex > 2) menuIndex = 0;
  }

  // 가운데 버튼
  if (lastMid == HIGH && currentMid == LOW) {
    executeAction();
  }

  lastLeft = currentLeft;
  lastRight = currentRight;
  lastMid = currentMid;
}

void executeAction() {
  if (state == DEAD) {
    hunger = 100;
    happy = 100;
    energy = 100;
    level = 1;
    state = EGG;
    eggStartTime = millis();
    saveGame();
    return;
  }
  if (menuIndex == 2) {   // CLEAN
    poop = false;
    happy += 10;
    poopTimer = millis();
  }

  switch (menuIndex) {
    case 0: // 먹이
      hunger += 20;
      if (hunger > 100) hunger = 100;
      break;

    case 1: // 놀기
      happy += 15;
      energy -= 10;
      break;

    case 2: // 잠
      energy += 25;
      break;
  }
}

void restartGame() {
  hunger = 100;
  happy = 100;
  energy = 100;
  state = MENU;
}

void drawPet() {
  int size;
  int eyeOffset;
  //int y = 36; //pet 바닥에
  if (evolution == 0) {
    size = 6;   // 아기
    eyeOffset = 3;
  }
  if (evolution == 1) {
    size = 10;  // 어린이
    eyeOffset = 5;
  }
  if (evolution == 2) {
    size = 14;  // 성체
    eyeOffset = 6;
  }
  if (evolution == 3) {
    size = 16;  // 극성체
    eyeOffset = 8;
  }

  //눈위치 진화때마다 바꿔줌
  //if (evolution == 0) eyeOffset = 3;
  //if (evolution == 1) eyeOffset = 5;
  //if (evolution == 2) eyeOffset = 6;
  //if (evolution == 3) eyeOffset = 7;
  
  if (millis() - lastAnim > 400) {
    animFrame = !animFrame;
    lastAnim = millis();
  }
  //이거좀 이상하다. 주석
  display.fillCircle(64-eyeOffset, 30, 2, SSD1306_BLACK);
  display.fillCircle(64+eyeOffset, 30, 2, SSD1306_BLACK);
  Mood mood = getMood();
  //level에 따른 성장 표현
//  size = (level == 1) ? 8 : 12;
  display.fillCircle(64, 36, size, SSD1306_WHITE);
  // 얼굴을 흰색으로 채움
  //display.fillCircle(64, 32, 12, SSD1306_WHITE);
    
  int eyeY = animFrame ? 28 : 30;
  
  switch (mood) {

    case HAPPY:
      display.fillCircle(59, eyeY, 2, SSD1306_BLACK);
      display.fillCircle(69, eyeY, 2, SSD1306_BLACK);
      display.drawLine(58, 36, 70, 36, SSD1306_BLACK);
      break;

    case HUNGRY:
      display.fillCircle(59, eyeY, 2, SSD1306_BLACK);
      display.fillCircle(69, eyeY, 2, SSD1306_BLACK);
      display.drawLine(58, 38, 70, 34, SSD1306_BLACK);
      break;

    case SLEEPY:
      display.drawLine(57, eyeY, 61, eyeY, SSD1306_BLACK);
      display.drawLine(67, eyeY, 71, eyeY, SSD1306_BLACK);
      display.drawLine(60, 36, 68, 36, SSD1306_BLACK);
      break;

    default:
      display.fillCircle(59, eyeY, 2, SSD1306_BLACK);
      display.fillCircle(69, eyeY, 2, SSD1306_BLACK);
      display.drawLine(60, 36, 68, 36, SSD1306_BLACK);
      break;
  }
}

void drawScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.fillCircle(64, 32, 10, SSD1306_WHITE);
  
  updateWorld();
  drawBackground();

  display.setTextSize(1);
  //맨위에 나이 표시
  display.setCursor(0,0);
  display.print("Age:");
  display.print(age);
  //아래 상태표시
  //display.setCursor(0,10);
  display.print(" H:");
  display.print(hunger);
  display.print(" E:");
  display.print(energy);

  if (state == EGG) {

  display.clearDisplay();

  unsigned long elapsed = millis() - eggStartTime;

  int crackStage = 0;
  if (elapsed > 1500) crackStage = 1;
  if (elapsed > 3000) crackStage = 2;

  // 흔들림 강도 점점 증가
  int shakePower = map(elapsed, 0, 5000, 0, 4);
  int shake = random(-shakePower, shakePower);

  // 알 그리기
  display.fillCircle(64 + shake, 34, 10, SSD1306_WHITE);
  display.fillCircle(64 + shake, 28, 8, SSD1306_WHITE);

  // 금
  if (crackStage >= 1)
    display.drawLine(64+shake, 26, 60+shake, 34, SSD1306_BLACK);

  if (crackStage >= 2) {
    display.drawLine(60+shake, 34, 68+shake, 40, SSD1306_BLACK);
    display.drawLine(68+shake, 40, 64+shake, 46, SSD1306_BLACK);
  }

  // 부화 직전 깜빡임
  if (elapsed > 4500) {
    if (millis() % 200 < 100)
      display.invertDisplay(true);
    else
      display.invertDisplay(false);
    }

    // 5초 지나면 부화
    if (elapsed > 5000) {
      delay(200);//리셋할때 가끔 화면색이 반전되어 나옴. 살짝 delay를..
      display.invertDisplay(false); 
      tone(BUZZER, 1200, 150);
      delay(200);
      tone(BUZZER, 1800, 200);
      state = MENU;
    }
    
    //위에 처리 후.
    display.display();

    return;
  }
  
  if (state == DEAD) {

    display.clearDisplay();

    // 무덤 모양
    display.fillRect(56, 30, 16, 20, SSD1306_WHITE);
    display.fillCircle(64, 30, 8, SSD1306_WHITE);

    display.setTextColor(SSD1306_BLACK);
    display.setCursor(60, 34);
    display.print("RIP");

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 55);
    display.print("Press MID");

    display.display();
    return;
  }

  drawPet();
  //똥그리기
  if (poop) {
    display.fillCircle(80, 48, 3, SSD1306_WHITE);
    display.fillCircle(80, 45, 2, SSD1306_WHITE);
  }

  display.setCursor(10, 55);

  if (menuIndex == 0) display.print(">FEED ");
  else display.print(" FEED ");

  if (menuIndex == 1) display.print(">PLAY ");
  else display.print(" PLAY ");

  if (menuIndex == 2) display.print(">SLEEP");
  else display.print(" SLEEP");

  display.display();
}
//밤낮전환 함수
void updateWorld(){

  if(millis() - worldTimer > 20000){ // 20초마다 변경
    isDay = !isDay;
    worldTimer = millis();
  }
}
//배경그리기
void drawBackground(){

  if(isDay){

    // 태양
    display.drawCircle(110,10,6,WHITE);

    // 구름
    display.drawCircle(20,12,4,WHITE);
    display.drawCircle(25,12,4,WHITE);
    display.drawCircle(30,12,4,WHITE);

  }else{

    // 달
    display.drawCircle(110,10,6,WHITE);
    display.fillCircle(113,10,6,BLACK);

    // 별
    display.drawPixel(20,10,WHITE);
    display.drawPixel(40,6,WHITE);
    display.drawPixel(70,12,WHITE);
    display.drawPixel(90,5,WHITE);
    display.drawPixel(50,14,WHITE);

  }

  // 바닥
  display.drawLine(0,48,128,48,WHITE);

}
