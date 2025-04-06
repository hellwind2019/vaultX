#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

//-------НАЛАШТУВАННЯ---------
#define coin_amount 4          // кількість номіналів монет
float coin_value[coin_amount] = { 1.0, 2.0, 5.0, 10.0};  // номінали монет
String currency = "UAH"; 


int goal = 0; // ціль
int coin_signal[coin_amount] = {44, 380, 800, 960};   // Збережені значення для кожної монетки

int empty_signal;               // рівень пустого сигналу
unsigned long  reset_timer; // таймеры
float summ_money = 0;            // накопичена сума

//Адреси в EEPROM
#define SUM_ADR 20 
#define GOAL_ADR 30


LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo l_servo;
Servo r_servo;

boolean first_flag = true, moveServo_flag = false, settings_flag = false;   // флажки
//-------ПОРТИ---------
#define RIGHT_BUTTON  3   //Права кнопка
#define LEFT_BUTTON   2   //Ліва кнопка
#define IRpin         17  //ІЧ світлодіод
#define IRsens        14  //Фототранизистор
#define LEFT_SERVO    7   //Лівий сервопривід
#define RIGHT_SERVO   8   //Правий сервопривід
#define RELAY         10  //Керування реле
//-------ПОРТИ---------

//------КУТИ СЕРВО-----
#define L_MAX         60
#define L_MIN         0
#define L_MID         30

#define R_MAX         120
#define R_MIN         155
#define R_MID         145
//------КУТИ СЕРВО-----

int ir_signal, last_signal;
boolean coin_flag = false;

void setup() {
  Serial.begin(9600);
  init_servo();
  greeting();

  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(IRpin, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, 0);
  digitalWrite(IRpin, 1);


  empty_signal = analogRead(IRsens);
  Serial.println(empty_signal);
  if (!digitalRead(LEFT_BUTTON)) {  // якщо утримаємо кнопку скидання
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hold 2s for ");
    lcd.setCursor(0, 1);
    lcd.print("Clearing memory");
    delay(500);
    reset_timer = millis();
    while (1) {
      if (millis() - reset_timer > 3000) {        // якщо утримали 3с
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Memory cleared");
        lcd.setCursor(0, 1);
        lcd.print("Release RIGHT_BUTTON");
        delay(100);
      }
      if (digitalRead(LEFT_BUTTON)) {   // кнопка відпущена, очистити пам'ять
        EEPROM.write(SUM_ADR, 0);
        EEPROM.write(GOAL_ADR, 0);
        lcd.clear();
        break;
      }

    }
    for (byte i = 0; i < coin_amount; i++) {
      Serial.print(coin_value[i]);
      Serial.print(" : ");
      Serial.println(coin_signal[i]);
    }
  }
  summ_money = EEPROM.read(SUM_ADR);
  goal = EEPROM.read(GOAL_ADR);

}

void loop() {
  if (first_flag) {
    delay(500);
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Put Money ->");
    lcd.setCursor(0, 1); lcd.print(summ_money);
    lcd.setCursor(13, 1); lcd.print(currency);
    empty_signal = analogRead(IRsens);

  }

  last_signal = empty_signal;
  while (1) {
    if (!digitalRead(RIGHT_BUTTON)) {
      settingMenu();
    }
     //Алгорим розпізнавання монетки
    ir_signal = analogRead(IRsens);
    if (ir_signal > last_signal) last_signal = ir_signal; 
    if (ir_signal - empty_signal > 5 ) coin_flag = true;
    if (coin_flag && (abs(ir_signal - empty_signal)) < 5) {

      for (byte i = 0; i < coin_amount; i++) {
        int diff = abs(last_signal - coin_signal[i]);
        if (diff < 30) {
          summ_money += coin_value[i];
          lcd.setCursor(0, 1); lcd.print(summ_money);
          if (summ_money >= goal) remove_lock();
          EEPROM.write(SUM_ADR, summ_money);
          moveServos();
          break;
        }
      }
      coin_flag = false;
      break;
    }
  }
}
void settingMenu() { //Меню налаштувань цілі
  printGoal();
  long tim1 = millis();
  while (millis() - tim1 < 2000) {
    if (!digitalRead(RIGHT_BUTTON) && goal < 10000) {
      goal += 10;
      printGoal();
      tim1 = millis();
      delay(200);
    }
    if (!digitalRead(LEFT_BUTTON) && goal > 10) {
      goal -= 10;
      printGoal();
      tim1 = millis();
      delay(200);
    }
  }
  EEPROM.write(GOAL_ADR, goal);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Put Money ->");
  lcd.setCursor(0, 1); lcd.print(summ_money);
  lcd.setCursor(13, 1); lcd.print(currency);


}
void printGoal() { //Вивід цілі на екран
  lcd.clear(); lcd.setCursor(4, 0);
  lcd.print("Goal: ");
  lcd.print(goal);
  lcd.setCursor(0 , 1);
  lcd.print("Wait 2s for exit");
}
void greeting() { //Привітальний текст та інструкції
  lcd.init();
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print("VaultX");
  delay(500);
  lcd.clear();
  lcd.print("Hold L :reset");
  lcd.setCursor(0, 1);
  lcd.print("Hold R :set goal");
  delay(500);
}
void init_servo() { //Ініціалізація сервоприводів
  l_servo.attach(7);
  r_servo.attach(8);
  l_servo.write(L_MIN);
  r_servo.write(R_MIN);
  delay(500);
  l_servo.write(L_MAX);
  r_servo.write(R_MAX);
  delay(500);
  l_servo.write(L_MID);
  r_servo.write(R_MID);
}
void moveServos() { //Рух сервоприводів

  l_servo.write(L_MIN);
  r_servo.write(R_MAX);
  delay(300);
  l_servo.write(L_MAX);
  r_servo.write(R_MIN);
  delay(300);
  l_servo.write(L_MID);
  r_servo.write(R_MID);
}
void remove_lock() { //Прибирання замка
  summ_money = 0;
  EEPROM.write(SUM_ADR, summ_money);

  digitalWrite(RELAY, 1);
  lcd.clear();
  lcd.print("CONGRATULATIONS!");
  l_servo.write(L_MIN);
  r_servo.write(R_MAX);
  delay(200);
  l_servo.write(L_MAX);
  r_servo.write(R_MIN);
  delay(200);
  l_servo.write(L_MID);
  r_servo.write(R_MID);

  lcd.setCursor(0, 1);
  lcd.print("You have: ");
  lcd.print(goal);
  delay(5000);
  digitalWrite(RELAY, 0);
}
