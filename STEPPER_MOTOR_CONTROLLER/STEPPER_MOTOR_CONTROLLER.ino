// === Пины управления двигателем ===
#define MOTOR_IN1 2
#define MOTOR_IN2 3
#define MOTOR_IN3 4
#define MOTOR_IN4 5
#define MOTOR_ENA 9
#define MOTOR_ENB 10

// === Кнопки и переключатели ===
#define BTN_ONE_STEP    13
#define SW_DIR      12
#define SW_HALFSTEP 11
#define SW_AUTO     8
#define SW_SMOOTH   7
#define SW_SERVO    6
#define BTN_ZERO_ANGLE  A4

// === Потенциометры ===
#define POT_SPEED    A0
#define POT_PWM      A1
#define POT_IMPULSE  A2
#define POT_ANGLE    A3

// === Глобальные переменные ===
bool motorDir = false;
bool modeHalf = false;
bool modeAuto = false;
bool initStep = true;
bool modeServo = false;
bool lastSmoothState = false;
bool lastBtnStep = false;

int pwmLevel = 0;
int stepIndex = 0;
float angleNow = 0;
float angleGoal = 0;

void setup() {
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);

  pinMode(BTN_ONE_STEP, INPUT);
  pinMode(SW_DIR, INPUT);
  pinMode(SW_HALFSTEP, INPUT);
  pinMode(SW_AUTO, INPUT);
  pinMode(SW_SMOOTH, INPUT);
  pinMode(SW_SERVO, INPUT);
  pinMode(BTN_ZERO_ANGLE, INPUT);

  Serial.begin(9600);
}

// === Один шаг шагового двигателя ===
void doStep() {
  if (!initStep) {
    int jump = modeHalf ? 1 : 2;
    float deg = modeHalf ? 1.5 : 3;

    stepIndex = motorDir ? (stepIndex + jump) % 8 : (stepIndex - jump + 8) % 8;
    angleNow += motorDir ? deg : -deg;

  } else {
    initStep = false;
  }

  analogWrite(MOTOR_ENA, pwmLevel);
  analogWrite(MOTOR_ENB, pwmLevel);

  Serial.print("Step: ");
  Serial.println(stepIndex);
  Serial.print("Angle: ");
  Serial.println(angleNow);

  PORTD &= ~B00111100;
  switch (stepIndex) {
    case 0: PORTD |= B00000100; break;
    case 1: PORTD |= B00010100; break;
    case 2: PORTD |= B00010000; break;
    case 3: PORTD |= B00011000; break;
    case 4: PORTD |= B00001000; break;
    case 5: PORTD |= B00101000; break;
    case 6: PORTD |= B00100000; break;
    case 7: PORTD |= B00100100; break;
  }
}

// === Плавный шаг ===
void stepSmooth() {
  pwmLevel = modeHalf ? 62 : 102;
  int minDelay = modeHalf ? 8 : 16;

  for (int t = 208; t > minDelay; t -= 8, pwmLevel += 6) {
    pulseWithDelay(t, 75);
  }
  pwmLevel = 255;
}

// === Шаг с импульсом ===
void pulseWithDelay(int delayStep, int widthPercent) {
  float tOn  = delayStep * (widthPercent / 100.0);
  float tOff = delayStep - tOn;

  Serial.print("Pulse ON: ");
  Serial.print(tOn);
  Serial.print(" ms, OFF: ");
  Serial.println(tOff);

  doStep();
  delay(tOn);
  PORTD &= ~B00111100;
  delay(tOff);
}

// === Основной цикл ===
void loop() {

  motorDir   = digitalRead(SW_DIR);
  modeHalf   = digitalRead(SW_HALFSTEP);
  modeAuto   = digitalRead(SW_AUTO);
  bool smoothFlag = digitalRead(SW_SMOOTH);
  modeServo  = digitalRead(SW_SERVO);
  bool zeroFlag = digitalRead(BTN_ZERO_ANGLE);
  bool btnStep = digitalRead(BTN_ONE_STEP);

  int valSpeed   = analogRead(POT_SPEED);
  int valPWM     = analogRead(POT_PWM);
  int valImpulse = analogRead(POT_IMPULSE);
  int valAngle   = analogRead(POT_ANGLE);

  angleGoal = map(valAngle, 0, 1023, 300, 0);
  int delayStep = map(valSpeed, 0, 1023, 1000, 10);
  int pulseWidth = map(valImpulse, 0, 1023, 100, 0);

  if (zeroFlag) angleNow = 0;

  // === Плавный шаг ===
  if (smoothFlag && !lastSmoothState) {
    stepSmooth();
  } else if (!smoothFlag) {
    pwmLevel = map(valPWM, 0, 1023, 255, 0);
  }

  // === Серво-режим ===
  if (modeServo) {

    int rawAngle = analogRead(POT_ANGLE);
    angleGoal = map(rawAngle, 0, 1023, 300, 0);
    if (!motorDir) angleGoal *= -1;

    float diff = angleGoal - angleNow;

    if (abs(diff) >= 3) {
      motorDir = diff > 0;
      modeHalf = abs(diff) < 3;
      pulseWithDelay(delayStep, pulseWidth);
    } else {
      pwmLevel = 51;
      analogWrite(MOTOR_ENA, pwmLevel);
      analogWrite(MOTOR_ENB, pwmLevel);
    }
  }

  // === Одиночный шаг ===
  else if (btnStep && !lastBtnStep) {
    PORTD &= ~B00111100;
    doStep();
  }

  // === Автоматический шаг ===
  else if (modeAuto || lastSmoothState) {
    int d = lastSmoothState ? 10 : delayStep;
    int pw = lastSmoothState ? 75 : pulseWidth;
    pulseWithDelay(d, pw);
  }

  lastBtnStep = btnStep;
  lastSmoothState = smoothFlag;
}
