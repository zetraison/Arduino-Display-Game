#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <Wire.h>

const int LCD_I2C_ADDRESS = 0x3F;

// Arduino remote controller
const long int BUTTON_CH        = 0xFF629D;    // CH
const long int BUTTON_START     = 0xFF30CF;    // BUTTON 1
const long int BUTTON_LEFT      = 0xFF10EF;    // BUTTON 4
const long int BUTTON_RIGHT     = 0xFF5AA5;    // BUTTON 6
const long int BUTTON_UP        = 0xFF18E7;    // BUTTON 2
const long int BUTTON_DOWN      = 0xFF4AB5;    // BUTTON 8
const long int BUTTON_NEXT      = 0xFF02FD;    // NEXT
const long int BUTTON_PREV      = 0xFF22DD;    // PREV
const long int BUTTON_VOL_PLUS  = 0xFFA857;    // VOL+
const long int BUTTON_VOL_MINUS = 0xFFE01F;    // VOL-
const long int BUTTON_PUSHED    = 0xFFFFFFFF;  // ALL PUSHED BUTTONS

const byte DIRECTION_RIGHT  = 0;
const byte DIRECTION_LEFT   = 1;
const byte DIRECTION_UP     = 2;
const byte DIRECTION_DOWN   = 3;

const byte BORDER_RIGHT   = 15;
const byte BORDER_LEFT    = 0;
const byte BORDER_UP      = 0;
const byte BORDER_DOWN    = 1;

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

byte player_x, player_y, bot_1_x, bot_1_y, bot_2_x, bot_2_y, last_direction;
boolean is_start, is_lose, bot_death;
int tick, passed_bot, bot_speed, level;
long int last_pressed_button;

IRrecv irrecv(RECV_PIN);
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS,16,2);

decode_results results;

void print_start_screen() {
  lcd.setCursor(2, 0);
  lcd.print("Street Racer");
  lcd.setCursor(0, 1);
  lcd.print("Press 1 to start");
}

void print_lose_screen() {
  lcd.setCursor(4, 0);
  lcd.print("You lose!");
  lcd.setCursor(0, 1);
  lcd.print("Press 1 to start");
}

void print_player(byte x, byte y) {
  lcd.setCursor(x, y);
  lcd.print("\1");
}

void print_bot(byte x, byte y) {
  lcd.setCursor(x, y);
  lcd.print("x");
}

void print_level() {
  lcd.setCursor(4, 0);
  lcd.print("level_");
  if (level < 10) {
    lcd.setCursor(10, 0);
  } else {
    lcd.setCursor(11, 0);
  }
  lcd.print(level);
  delay(1500);
}

void print_result() {
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

void repeat_direction() {
     switch (last_direction) {
      case DIRECTION_RIGHT:
        if (player_x < BORDER_RIGHT) player_x++;
        break;
      case DIRECTION_LEFT:
        if (player_x > BORDER_LEFT) player_x--;
        break;
      case DIRECTION_UP:
        if (player_y != BORDER_UP) player_y--;
        break;
      case DIRECTION_DOWN:
        if (player_y != BORDER_DOWN) player_y++;
        break;
    }
}

void init_game() {
    player_x  = BORDER_LEFT;
    player_y  = BORDER_UP;
    bot_1_x   = BORDER_RIGHT;
    bot_1_y   = BORDER_UP;
    bot_2_x   = BORDER_LEFT-1;
    bot_2_y   = BORDER_DOWN;
    is_start  = false;
    is_lose   = false;
    bot_death = false;
    last_direction  = 0;
    bot_speed       = 10;
    passed_bot      = 1;
    level           = 1;
};

void setup() {
  Serial.begin(9600);
  
  irrecv.enableIRIn();
  pinMode(TONE_PIN, OUTPUT);
  
  lcd.begin();          
  lcd.backlight();
  lcd.createChar(1, PLAYER);
  is_start = true;
}

void loop() {
  
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    
    switch(results.value) {
       case BUTTON_START:
        init_game();
        break;
      case BUTTON_RIGHT:
        if (player_x < BORDER_RIGHT) player_x++;
        last_direction = DIRECTION_RIGHT;
        break;
      case BUTTON_LEFT:
        if (player_x > BORDER_LEFT) player_x--;
        last_direction = DIRECTION_LEFT;
        break;
      case BUTTON_UP:
        if (player_y != BORDER_UP) player_y--;
        last_direction = DIRECTION_UP;
        break;
      case BUTTON_DOWN:
        if (player_y != BORDER_DOWN) player_y++;
        last_direction = DIRECTION_DOWN;
        break;
      case BUTTON_VOL_PLUS:
        if (bot_speed < 100) bot_speed++;
        break;
      case BUTTON_VOL_MINUS:
        if (bot_speed > 1) bot_speed--;
        break;
     case BUTTON_PUSHED:
        if (last_pressed_button == (BUTTON_RIGHT || BUTTON_LEFT || BUTTON_UP || BUTTON_DOWN))
          repeat_direction();
        break;
    };
    
    last_pressed_button = results.value;
    
    irrecv.resume(); // Receive the next value
  }
  
  if (is_start) {
    print_start_screen();
    return;
  };
  
  if (is_lose) {
    print_lose_screen();
    return;
  }
  
  print_bot(bot_1_x, bot_1_y);
  print_bot(bot_2_x, bot_2_y);
  print_player(player_x, player_y);
  print_result();
  
  if (tick % (100 / bot_speed) == 0) {
    bot_1_x--;
    bot_2_x--;
    
    if (bot_1_x == BORDER_LEFT || bot_2_x == BORDER_LEFT) {
      passed_bot++;
      bot_death = true;
    }
  }
  
  if (passed_bot % 10 == 0 && bot_death) {
    bot_speed++;
    print_level();
    bot_death = false;
    level++;
  }
  
  if (bot_1_x == BORDER_LEFT + BORDER_RIGHT / 2) {
    bot_2_x = BORDER_RIGHT;
  }
  
  if (bot_2_x == BORDER_LEFT + BORDER_RIGHT / 2) {
    bot_1_x = BORDER_RIGHT;
  }
  
  if (bot_1_x == player_x && bot_1_y == player_y || bot_2_x == player_x && bot_2_y == player_y) {
    is_lose = true;
  };
  
  delay(10);
  lcd.clear();
  tick++;
}
