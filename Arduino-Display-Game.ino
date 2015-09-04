#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <Wire.h>

const int LCD_I2C_ADDRESS = 0x3F;

// Arduino remote controller
const long int BUTTON_CH        = 0xFF629D;    // CH
const long int BUTTON_CH_MINUS  = 0xFFA25D;    // CH-
const long int BUTTON_CH_PLUS   = 0xFFE21D;    // CH+
const long int BUTTON_START     = 0xFF30CF;    // BUTTON 1
const long int BUTTON_LEFT      = 0xFF10EF;    // BUTTON 4
const long int BUTTON_RIGHT     = 0xFF5AA5;    // BUTTON 6
const long int BUTTON_UP        = 0xFF18E7;    // BUTTON 2
const long int BUTTON_DOWN      = 0xFF4AB5;    // BUTTON 8
const long int BUTTON_SHOOT     = 0xFF38C7;    // BUTTON 5
const long int BUTTON_NEXT      = 0xFF02FD;    // NEXT
const long int BUTTON_PREV      = 0xFF22DD;    // PREV
const long int BUTTON_VOL_PLUS  = 0xFFA857;    // VOL+
const long int BUTTON_VOL_MINUS = 0xFFE01F;    // VOL-
const long int BUTTON_EQ        = 0xFF906F;    // EQ
const long int BUTTON_PUSHED    = 0xFFFFFFFF;  // ALL PUSHED BUTTONS

const byte GAME_STREET_RACER    = 0;
const byte GAME_SHOOTER         = 1;

const byte DIRECTION_RIGHT  = 0;
const byte DIRECTION_LEFT   = 1;

const byte BORDER_RIGHT   = 15;
const byte BORDER_LEFT    = 0;
const byte BORDER_UP      = 0;
const byte BORDER_DOWN    = 1;

const byte PASSED_BOT_FOR_NEXT_LEVEL = 10;
const byte DEAD_BOT_FOR_NEXT_LEVEL   = 10;

const byte RECV_PIN  = 2;
const byte TONE_PIN  = 4;

byte PLAYER[8] =
{
  B00000,
  B11110,
  B00010,
  B00011,
  B00010,
  B11110,
  B00000,
};

byte BOT[8] =
{
  B00000,
  B01111,
  B01000,
  B11000,
  B01000,
  B01111,
  B00000,
};

byte player_x, player_y;    // Координаты игрока
byte bot_1_x, bot_1_y;      // Координаты бота №1
byte bot_2_x, bot_2_y;      // Координаты бота №2
byte shoot_x, shoot_y;      // Координаты снаряда
byte last_direction, game_settings;
boolean is_menu, is_game, is_lose, is_shoot;
boolean bot_is_passed, bot_is_dead;
int tick, passed_bot, dead_bot, bot_speed, level;
long int last_pressed_button, frequency;

IRrecv irrecv(RECV_PIN);
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS,16,2);

decode_results results;

// Вывод меню
void print_menu() {
  lcd.setCursor(1, 0);
  lcd.print(" -= Games =- ");
  switch (game_settings) {
    case GAME_STREET_RACER:
      lcd.setCursor(0, 1);
      lcd.print("< Street Racer >");
      break;
    case GAME_SHOOTER:
      lcd.setCursor(0, 1);
      lcd.print("<   Shooter    >");
      break;
  }
}

// Вывод сообщения о проигрыша
void print_lose_screen() {
  lcd.setCursor(4, 0);
  lcd.print("You lose!");
  lcd.setCursor(0, 1);
  lcd.print("Press 5 to start");
}

// Отрисовка игрока
void print_player(byte x, byte y) {
  lcd.setCursor(x, y);
  lcd.print("\1");
}

// Отрисовка ботов
void print_bot(byte x, byte y) {
  lcd.setCursor(x, y);
  lcd.print("\2");
}

// Отрисовка снаряда
void print_shoot(byte x, byte y) {
  lcd.setCursor(x, y);
  lcd.print("*");
}

// Вывод сообщения о сдедующем уровне
void print_level() {
  lcd.setCursor(4, 0);
  lcd.print("level ");
  if (level < 10) {
    lcd.setCursor(10, 0);
  } else {
    lcd.setCursor(11, 0);
  }
  lcd.print(level);
}

// Вывод сообщения о проиденных ботах
void print_passed_bot() {
  if (passed_bot < 10) {
    lcd.setCursor(15, 0);
  }
  
  if (passed_bot >= 10 && passed_bot < 100) {
    lcd.setCursor(14, 0);
  }
  
  if (passed_bot >= 100) {
    lcd.setCursor(13, 0);
  }
  lcd.print(passed_bot);
}

// Вывод сообщения о проиденных ботах
void print_dead_bot() {
  if (dead_bot < 10) {
    lcd.setCursor(15, 0);
  }
  
  if (dead_bot >= 10 && dead_bot < 100) {
    lcd.setCursor(14, 0);
  }
  
  if (dead_bot >= 100) {
    lcd.setCursor(13, 0);
  }
  lcd.print(dead_bot);
}

// Сигнал шага игрока
void signal_step() {
  tone(TONE_PIN, 400);
  delay(30);
  noTone(TONE_PIN);
}

// Сигнал стрельбы
void signal_shoot() {
  tone(TONE_PIN, 800);
  delay(50);
  noTone(TONE_PIN);
}

// Сигнал проинрыша
void signal_lose() {
  tone(TONE_PIN, 600);
  delay(200);
  tone(TONE_PIN, 400);
  delay(200);
  tone(TONE_PIN, 200);
  delay(500);
  noTone(TONE_PIN);
}

// Сигнал проинрыша
void signal_next_level() {
  tone(TONE_PIN, 300);
  delay(100);
  tone(TONE_PIN, 200);
  delay(100);
  tone(TONE_PIN, 400);
  delay(100);
  tone(TONE_PIN, 300);
  delay(100);
  tone(TONE_PIN, 500);
  delay(100);
  tone(TONE_PIN, 400);
  delay(300);
  noTone(TONE_PIN);
}

// Продолжить движение игрока при зажатой кнопке движения
void repeat_direction() {
     switch (last_direction) {
      case DIRECTION_RIGHT:
        if (player_x < BORDER_RIGHT) player_x++;
        break;
      case DIRECTION_LEFT:
        if (player_x > BORDER_LEFT) player_x--;
        break;
    }
}

void init_game() {
    player_x        = BORDER_LEFT;
    player_y        = BORDER_UP;
    bot_1_x         = BORDER_RIGHT+1;
    bot_1_y         = BORDER_UP;
    bot_2_x         = BORDER_RIGHT+1+BORDER_RIGHT/2;
    bot_2_y         = BORDER_DOWN;
    shoot_x         = BORDER_LEFT-1;
    shoot_y         = player_y;
    
    is_menu         = false;
    is_game         = true;
    is_lose         = false;
    is_shoot        = false;
    
    bot_is_passed   = false;
    bot_is_dead     = false;
    passed_bot      = 0;
    dead_bot        = 0;
    bot_speed       = 5;
    
    level           = 1;
    last_direction  = DIRECTION_RIGHT;
};

void setup() {
  Serial.begin(9600);
  
  irrecv.enableIRIn();
  pinMode(TONE_PIN, OUTPUT);
  
  lcd.begin();          
  lcd.backlight();
  lcd.createChar(1, PLAYER);
  lcd.createChar(2, BOT);
  
  is_menu = true;
  game_settings = GAME_STREET_RACER;
  last_direction = 0;
}

void loop() {
  
  // Считываем данные получаемые IR датчиком
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    
    switch(results.value) {
      case BUTTON_CH:
        is_menu = true;
        break;
      case BUTTON_CH_MINUS:
      case BUTTON_CH_PLUS:
        game_settings = (game_settings + 1) % 2;
        break;
      case BUTTON_RIGHT:
        if (is_menu) {
          game_settings = (game_settings + 1) % 2;
        }
        if (is_game) {
          if (player_x < BORDER_RIGHT) player_x++;
          signal_step();
          last_direction = DIRECTION_RIGHT;
        }
        last_pressed_button = BUTTON_RIGHT;
        break;
      case BUTTON_LEFT:
        if (is_menu) {
          game_settings = (game_settings + 1) % 2;
        }
        if (is_game) {
          if (player_x > BORDER_LEFT) player_x--;
          signal_step();
          last_direction = DIRECTION_LEFT;
        }
        last_pressed_button = BUTTON_LEFT;
        break;
      case BUTTON_UP:
        if (is_game) {
          if (player_y != BORDER_UP) player_y--;
          signal_step();
        }
        last_pressed_button = BUTTON_UP;
        break;
      case BUTTON_DOWN:
        if (is_game) {
          if (player_y != BORDER_DOWN) player_y++;
          signal_step();
        }
        last_pressed_button = BUTTON_DOWN;
        break;
      case BUTTON_SHOOT:
        if (is_menu || is_lose) {
          init_game();
          break;
        }
        if (is_game && game_settings == GAME_SHOOTER) {
          is_shoot = true;
          shoot_x = player_x + 1;
          shoot_y = player_y;
          signal_shoot();
        }
        last_pressed_button = BUTTON_SHOOT;
        break;
     case BUTTON_PUSHED:
        if (last_pressed_button == BUTTON_RIGHT) {
          repeat_direction();
          signal_step();
        }
        if (last_pressed_button == BUTTON_LEFT) {
          repeat_direction();
          signal_step();
        }
        break;
    };
    
    //last_pressed_button = results.value;
    
    irrecv.resume(); // Receive the next value
  }
  
  // Вывод экрана меню игры
  if (is_menu) {
    print_menu();
    return;
  };
  
  // Вывод сообщения о поражение
  if (is_lose) {
    print_lose_screen();
    return;
  }
  
  if (is_game) {
    
    // Отрисовка игрока и ботов
    print_bot(bot_1_x, bot_1_y);
    print_bot(bot_2_x, bot_2_y);
    print_player(player_x, player_y);
  
    // Game №1 - GAME_SHOOTER
    if (game_settings == GAME_SHOOTER) {
      print_dead_bot();
      
      // Шаг бота с периодом (100 / bot_speed) * tick секунд, где tick == 10мс
      if (tick % (100 / bot_speed) == 0) {
        bot_1_x--;
        bot_2_x--;
        
        // Если бот достиг левой границы поля, то защитываем проигрыш
        if (bot_1_x == BORDER_LEFT || bot_2_x == BORDER_LEFT) {
          is_lose = true;
          signal_lose();
        }
      }
      
      // Если убито 10 ботов, то переходим на следующий уровень (увеличаем скорость движения ботов)
      if (dead_bot % DEAD_BOT_FOR_NEXT_LEVEL == 0 && bot_is_dead && !is_lose) {
        dead_bot++;
        print_level();
        signal_next_level();
        bot_is_dead = false;
        level++;
      }
      
      // Расчет координат движения снаряда
      if (tick % 2 == 0 && is_shoot) {
        shoot_x++;
      }
      
      // Если снаряд достиг правой границы поля, то прекращаем расчет координат снаряда
      if (shoot_x == BORDER_RIGHT) {
        is_shoot = false;
      }
      
      // Пробитие бота №1 снарядом
      if (shoot_x == bot_1_x && shoot_y == bot_1_y) {
        bot_1_x = BORDER_RIGHT+1;
        shoot_x = BORDER_LEFT - 1;
        is_shoot = false;
        bot_is_dead = true;
        dead_bot++;
      }
      
        // Пробитие бота №2 снарядом
      if (shoot_x == bot_2_x && shoot_y == bot_2_y) {
        bot_2_x = BORDER_RIGHT+1;
        shoot_x = BORDER_LEFT - 1;
        is_shoot = false;
        bot_is_dead = true;
        dead_bot++;
      }
      
      print_shoot(shoot_x, shoot_y);
    }
    
    // Game №2 - GAME_STREET_RACER
    if (game_settings == GAME_STREET_RACER) {
      print_passed_bot();
      
      // Шаг бота с периодом (100 / bot_speed) * tick секунд, где tick == 10мс
      if (tick % (100 / bot_speed) == 0) {
        bot_1_x--;
        bot_2_x--;
        
        // Если бот достиг левой границы поля, увеличиваем счетчик пройденных ботов на 1
        if (bot_1_x == BORDER_LEFT || bot_2_x == BORDER_LEFT) {
          passed_bot++;
          bot_is_passed = true;
        }
      }
      
      // Если пройдено 10 ботов без столкновений, то переходим на следующий уровень (увеличаем скорость движения ботов)
      if (passed_bot % PASSED_BOT_FOR_NEXT_LEVEL == 0 && bot_is_passed && !is_lose) {
        bot_speed++;
        print_level();
        signal_next_level();
        bot_is_passed = false;
        level++;
      }
      
      // Если бот №1 достиг середины поля, то запускаем бота №2
      if (bot_1_x == BORDER_RIGHT / 2) {
        bot_2_x = BORDER_RIGHT + 1;
      }
      
      // Если бот №2 достиг середины поля, то запускаем бота №1
      if (bot_2_x == BORDER_RIGHT / 2) {
        bot_1_x = BORDER_RIGHT + 1;
      }
    }
    
    // Если игрок столнулся с одним из ботов, то засчитываем поражение
    if (bot_1_x == player_x && bot_1_y == player_y || bot_2_x == player_x && bot_2_y == player_y) {
      is_lose = true;
      signal_lose();
    };
  }
  
  delay(10);
  lcd.clear();
  tick++;
}
