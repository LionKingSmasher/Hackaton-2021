#include <time.h>

#define CLK  3
#define MOSI 2
#define RES  14
#define CS   16
#define DS   15

#define RED 0xf800
#define C1 0x1592
#define C2 0x6974
#define C3 0x1972
#define BLUE 0x001f
#define GREEN 0x7E00
#define YELLOW 0xFFE0
#define WHITE 0xffff
#define BLACK 0x0000

#define MAX_COL        10
#define MAX_HORIZONTAL 11

#define START_BACK_X   7
#define START_BACK_Y   3

#define erase_tetris_block(T) draw_tetris_block(T, BLACK)

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u64;

typedef struct {
  u8 x;
  u8 y;
  u16 block;
  u16 tetris_block;
} tetris_t;

volatile tetris_t curr_t;

u8 background[12][12] = {
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
};
u8 game_over[12][12] = {
  {1,5,5,5,5,5,5,5,5,5,5,1},
  {1,5,1,1,5,5,5,3,3,3,3,1},
  {1,1,5,5,1,5,5,3,5,5,5,1},
  {1,1,5,5,1,5,5,3,3,5,5,1},
  {1,5,1,1,5,5,5,3,5,5,5,1},
  {1,5,5,5,5,5,5,5,5,5,5,1},
  {1,5,5,2,5,5,5,2,5,5,5,1},
  {1,5,5,5,2,5,2,5,5,5,5,1},
  {1,5,5,5,5,2,5,5,5,5,5,1},
  {1,5,5,5,5,5,5,5,5,5,5,1},
  {1,5,5,5,5,5,5,5,5,5,5,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
};
uint32_t curr_millis = 0, prev_millis = 0, joy_millis = 0;

u16 block_color[7] = {
  RED,
  BLUE,
  GREEN,
  YELLOW,
  C1,
  C2,
  C3
};

u16 block[7] = {
  0x0270,
  /* 
   *     #
   *    ###
   *   
   */
  0x0170,
  /* 
   *      #
   *    ###
   *   
   */
  0x0470,
  /* 
   *    # 
   *    ###
   *   
   */
  0x0630,
  /* 
   *    ##
   *     ##
   *   
   */
  0x0360,
  /* 
   *     ##
   *    ##
   *   
   */
  0x2222, // 야 쓰레기! 폭풍저그! 콩진호가 간다!!!
  /*     #
   *     #
   *     #
   *     #
   */
  0x0660, // 융융!!
  /* 
   *    ##
   *    ##
   *   
   */
};

u8 ssd1351_pins[5] = {
  CLK,  // CLOCK
  MOSI, // MOSI
  RES,  // Reset
  CS,   // Slave Select
  DS    // D/C
};

void send_u8(u8 data){
  for(register u8 i = 0x80; i > 0; i >>= 1){
    if(i & data)
      PORTD |= (0x01 << 1);
    else
      PORTD &= ~(0x01 << 1);

    PORTD |= 0x01;
    PORTD &= ~0x01;
  }
}

inline void ssd1351_command(u8 command){
  PORTB &= ~(0x01 << 2);
  PORTB &= ~(0x01 << 1);
  send_u8(command);
  PORTB |= (0x01 << 2);
  PORTB |= (0x01 << 1);
}

inline void ssd1351_data(u8 command) {
  PORTB &= ~(0x01 << 2);
  send_u8(command);
  PORTB |= (0x01 << 2);
}

void ssd1351_init(void){
  digitalWrite(CS, LOW);

  digitalWrite(RES, LOW);
  delay(100);
  digitalWrite(RES, HIGH);
  delay(100);

  ssd1351_command(0xfd);   // coommand lock
  ssd1351_data(0x12);      // Unlock OLED Driver
  ssd1351_command(0xfd);   // coommand lock
  ssd1351_data(0xB1);      // Command A2, B1, B3, BB, BE, C1 accessible
  
  ssd1351_command(0xAE);   // display off

  ssd1351_command(0xB3);
  ssd1351_data(0xF1);

  ssd1351_command(0xA0);   // re-map
  ssd1351_data(0x74);      // Horizontal Increment

  ssd1351_command(0xCA);   // Mux ratio set
  ssd1351_data(0x7F);    

  ssd1351_command(0xA2);   // display offset set
  ssd1351_data(0x00);      // display offset is 0

  ssd1351_command(0xB5);   // GPIO setting
  ssd1351_data(0x00);      // pin HiZ setting
  
  ssd1351_command(0xAB);   // Display Mode
  ssd1351_data(0x01);      // Enable V_DD regulator & 8bit parallel interface 

  ssd1351_command(0xB1);   // Set reset / Pre-charge period
  ssd1351_data(0x32);      // Phase 1 period of 5 CLKS & Phase 2 period of 3 CLKS

  ssd1351_command(0xBE);   // Set voltage
  ssd1351_data(0x05);      // 0.82 * V_cc

  ssd1351_command(0xA6);   // Reset Normal display

  ssd1351_command(0xC1);   // Set contraset current for color A,B,C
  ssd1351_data(0xC8);      // Color 1
  ssd1351_data(0x80);      // Color 2
  ssd1351_data(0xC8);      // Color 3

  ssd1351_command(0xC7);   // Master Constrast Current Control
  ssd1351_data(0x0f);      // no change

  ssd1351_command(0xB4);    // Set Low Volatge
  ssd1351_data(0xA0);      
  ssd1351_data(0xB5);
  ssd1351_data(0x55);

  ssd1351_command(0xB6);   // Set Second Pre-charge period
  ssd1351_data(0x01);      // 1 CLKS

  ssd1351_command(0xAF);   // Display on
}

void clear_screen(u16 color){
  u8 front_byte = (color >> 8) & 8;
  u8 back_byte  = color & 0xFF;

  ssd1351_command(0x15); // set column address
  ssd1351_data(0x00);    // Start Address : 0
  ssd1351_data(0x7F);    // End Address   : 0x7F

  ssd1351_command(0x75); // Set horizontal Address
  ssd1351_data(0x00);
  ssd1351_data(0x7F);
  

  ssd1351_command(0x5C); // write Ram Command
  
  for(register u8 i = 0; i < 128; i++){
    for(register u8 j = 0; j < 128; j++){
      ssd1351_data(front_byte);
      ssd1351_data(back_byte);
    }
  }
}


void put_pixel(u8 x, u8 y, u16 color) {
  u8 front_byte = (color & 0xff00) >> 8;
  u8 back_byte  = color & 0xFF;

  ssd1351_command(0x15);  // Set column address
  ssd1351_data(x);        // start : x
  ssd1351_data(x);        // end   : x

  ssd1351_command(0x75);  // Set horizontal Address
  ssd1351_data(y);        // start : y
  ssd1351_data(y);        // end   : y

  ssd1351_command(0x5C);  // Write Ram
  ssd1351_data(front_byte);
  ssd1351_data(back_byte);
}

void background_init(void){
  for(register u8 i = 0; i < 11; i++){
    for(register u8 j = 1; j < 11; j++){
      background[i][j] = 0;
    }
  }
}
//=======================================================================
void draw_block(u8 x, u8 y, u16 color){
  for(register u8 i = 0; i < 10; i++){
    for(register u8 j = 0; j < 10; j++){
      put_pixel(START_BACK_X + x * 11 + i, START_BACK_Y + y * 11 + j, color);
    }
  }
}

//void erase_tetris_block(tetris_t* t, u16 block){
//  for(register u8 i = 0; i < 4; i++){
//    for(register u8 j = 0; j < 4; j++){
//      if(block & (0x8000 >> (i * 4 + j)))
//        draw_block(START_BACK_X + t->x * 11 + i, START_BACK_Y + t->y * 11 + j, BLACK);
//    }
//  }
//}

void draw_tetris_block(tetris_t* t, u16 color){
  for(register u8 i = 0; i < 4; i++){
    for(register u8 j = 0; j < 4; j++){
      if(t->tetris_block & (0x8000 >> (i * 4 + j)))
        draw_block(t->x + j, t->y + i, color);
    }
  }
}

u8 line_check(int line){
  for(register u8 i = 1; i < 11; i++){
    if(background[line][i] == 0){
      return 0;
    }
  }
  return 1;
}

int overlap_check(tetris_t* t, char offset_x, char offset_y){
  for(register u8 i = 4; i > 0; i--){
    for(register u8 j = 4; j > 0; j--){
      if(background[t->y + 3 - (i - 1) + offset_y][t->x + 3 - (j - 1) + offset_x] > 0 && (t->tetris_block >> ((i-1) * 4 + (j-1))) & 0x01)
        return 1;
//      Serial.print(background[t->y + 3 - (i - 1)][t->x + 3 - (j - 1)] > 0);
    }
//    Serial.println();
  }
  return 0;
}

void draw_background(void){
  for(register u8 i = 0; i < 11; i++){
    for(register u8 j = 1; j < 11; j++){
      if(background[i][j] > 0)
        draw_block(j, i, block_color[background[i][j]-1]);
      else
        draw_block(j, i, BLACK);
    }
  }
}

void draw_game_over(void){
  for(register u8 i = 0; i < 11; i++){
    for(register u8 j = 1; j < 11; j++){
        draw_block(j, i, block_color[game_over[i][j]-1]);
    }
  }
}

void rotate_block(tetris_t* t) {
  volatile u16 tmp = t->tetris_block;
  tetris_t tmp_t   = *t;
  tmp = 
    (tmp & (0x01 << 3)) << 12   | // 15
    (tmp & (0x01 << 7)) << 7    | // 14
    (tmp & (0x01 << 11)) << 2   | // 13
    (tmp & (0x01 << 15)) >> 3   | // 12
    (tmp & (0x01 << 2)) << 9    | // 11
    (tmp & (0x01 << 6)) << 4    | // 10
    (tmp & (0x01 << 10)) >> 1   | // 9
    (tmp & (0x01 << 14)) >> 6   | // 8
    (tmp & (0x01 << 1)) << 6    | // 7
    (tmp & (0x01 << 5)) << 1    | // 6
    (tmp & (0x01 << 9)) >> 4    | // 5
    (tmp & (0x01 << 13)) >> 9   | // 4
    (tmp & (0x01))       << 3   | // 3
    (tmp & (0x01 << 4)) >> 2    | // 2
    (tmp & (0x01 << 8)) >> 7    | // 1
    (tmp & (0x01 << 12)) >> 12;  // 0
  tmp_t.tetris_block = tmp;
  if(!overlap_check(&tmp_t , 0, 0)){
    erase_tetris_block(t);
    t->tetris_block = tmp;
  }
}

void drop_func(void) {
  if(!overlap_check(&curr_t, 0, 1)){
    erase_tetris_block(&curr_t);
    curr_t.y += 1;
    draw_tetris_block(&curr_t, block_color[curr_t.block]);
  }
  else{
    insert_block(&curr_t);
    for(u8 i = 10; i > 0; i--){
      if(line_check(i)){
        for(register u8 k = i; k > 0; k--){
          for(register u8 j = 0; j < 12; j++){
            background[k][j-1] = background[k-1][j-1];
          }
        }
      }
    }
    draw_background();
    curr_t.x = 2;
    curr_t.y = 1;
    curr_t.block = rand() % 7;
    curr_t.tetris_block = block[curr_t.block];
    if(overlap_check(&curr_t, 0, 0)){
      draw_game_over();
      while(!(analogRead(A2) > 800));
      background_init();
      draw_background();
      draw_tetris_block(&curr_t, block_color[curr_t.block]);
    }
  }
}

void hard_drop(void){
  
}

void insert_block(tetris_t* t){
  for(register u8 i = 4; i > 0; i--){
    for(register u8 j = 4; j > 0; j--){
      if((t->tetris_block >> ((i-1) * 4 + (j-1)) & 0x01))
        background[t->y + 3 - (i - 1)][t->x + 3 - (j - 1)] = t->block + 1;
    }
  }
}

//=======================================================================

void setup() {
  // put your setup code here, to run once:
  for(char i : ssd1351_pins){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  pinMode(A0, INPUT);
  pinMode(6, INPUT);
  
  digitalWrite(RES, HIGH); // Reset Pin High

  curr_t.x = 2;
  curr_t.y = 1;
  curr_t.block = 0;
  curr_t.tetris_block = block[curr_t.block];

  ssd1351_init();
  clear_screen(BLACK);

  draw_tetris_block(&curr_t, RED);
}


void loop() {
  // put your main code here, to run repeatedly:
  srand(analogRead(A0));
  srand(analogRead(A0) * (rand() % 100));
  curr_millis = millis();
  if(curr_millis - prev_millis > 1000){
    drop_func();
    prev_millis = curr_millis;
  }

  if(analogRead(A1) > 800){
    if(!overlap_check(&curr_t, 1, 0)){
      erase_tetris_block(&curr_t);
      curr_t.x += 1;
      draw_tetris_block(&curr_t, block_color[curr_t.block]);
    }
  }
  else if(analogRead(A1) < 300){
    if(!overlap_check(&curr_t, -1, 0)){
      erase_tetris_block(&curr_t);
      curr_t.x -= 1;
      draw_tetris_block(&curr_t, block_color[curr_t.block]);
    }
  }

  if(analogRead(A2) < 300){
    if(!overlap_check(&curr_t, 0, 1)){
      erase_tetris_block(&curr_t);
      curr_t.y += 1;
      draw_tetris_block(&curr_t, block_color[curr_t.block]);
    }
  }

  if(analogRead(A2) > 1010){
    if(curr_millis - joy_millis > 500){
      rotate_block(&curr_t);
      joy_millis = curr_millis;
    }
  }
}
