/*
 * fir_semester_work.c
 *
 * Created: 2020-07-09 오후 6:44:22
 * Author : Yunseo Hwang
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "lcd1602a_h68.h"

#define SW_DDR DDRA
#define SW_PIN PINA
#define HEART_CHAR 4
#define PLAYER_CHAR 5
#define BV(n) (1 << n)
#define CHECK_SW(n) (~SW_PIN & BV(n))

void lcd_set();
void lcd_item_set();
void lcd_player_set();
void lcd_player_up();
void lcd_player_down();
void lcd_player_draw();
int lcd_create_item();
int lcd_item_shift();

void fnds_set();
void fnd_on(unsigned char i, unsigned char num);
void fnds_on(unsigned int num);
void fnd_game_grade(uint8_t grade);

void button_set();

void game_start();
void game_finish(uint8_t is_win);
void game_set();
void game_pause();
void game_difficulty_led_set();
void game_difficulty_up();
uint8_t get_grade_option();
uint8_t get_game_grade();

uint8_t obstacle_pattern[5][8] = {
	{ 0x00, 0x04, 0x04, 0x06, 0x0E, 0x0F, 0x1F, 0x00 },
	{ 0x00, 0x00, 0x04, 0x0C, 0x0E, 0x1F, 0x1F, 0x00 },
	{ 0x1F, 0x1F, 0x0E, 0x06, 0x06, 0x04, 0x04, 0x00 },
	{ 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x04, 0x04, 0x00 },
};
uint8_t heart_pattern[8] = { 0x00, 0x0A, 0x15, 0x11, 0x11, 0x0A, 0x04, 0x00 }; 
uint8_t player_pattern[8] = { 0x0E, 0x0E, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x00 };
uint8_t player_is_up = 0;
uint8_t item_type = -1;
uint8_t item_x = LCD_ROWS_MAX - 1;
uint8_t item_y = 1;

int item_seed = 9425;
int item_seed_cnt = 0;
int item_speed = 50;

int game_score = 0;
uint8_t game_difficulty = 6;

int main(void)
{
	int i = 0;
	int before_grade = get_game_grade();
	
	lcd_set();
	fnds_set();
	lcd_item_set();
	lcd_player_set();
	button_set();
	game_set();
	game_start();
	
	srand(item_seed);
	
    while (1) 
    {
		if (CHECK_SW(0)) {
			lcd_player_up();
		} else if (CHECK_SW(1)) {
			lcd_player_down();
		} else if (CHECK_SW(2)) {
			game_start();
		} else if (CHECK_SW(3)) {
			game_pause();
		}
		
		item_seed++;
		if (item_seed == 9425) {
			item_seed = rand() % 9425;
		}
		
		if (item_seed_cnt == 9425) {
			item_seed_cnt = 0;
		}
		if (item_seed_cnt % 6 == 0)	{
			srand(item_seed);
		}
			
		if (item_x == LCD_ROWS_MAX - 1) {
			lcd_create_item();
			item_seed_cnt++;
		}
		lcd_item_shift();
		if (item_x == 0 && item_y == !player_is_up) {
			lcd_player_draw();
			
			if (item_type == 4) {
				game_score += 100;
				if (before_grade != get_game_grade()) {
					game_difficulty_up();
					if (get_game_grade() % 2 == 0 && get_game_grade() < 6) {
						item_speed -= 20;
					}
				}
				before_grade = get_game_grade();
				if (game_score >= 9900) {
					game_finish(1);
					game_set();
					before_grade = get_game_grade();
					continue;
				}
			} else {
				game_finish(0);
				game_set();
				before_grade = get_game_grade();
				continue;
			}
		}
		
		if (item_speed < 0) item_speed = 0;
		for (i = 0; i < item_speed; i++) {
			_delay_ms(1);
			if (CHECK_SW(0)) {
				lcd_player_up();
			} else if (CHECK_SW(1)) {
				lcd_player_down();
			}
		}
		if (item_x == 0) {
			if (item_y == player_is_up) {
				lcd_move(item_x, item_y);
				lcd_putc(' ');
			}
			item_x = LCD_ROWS_MAX - 1;
		}
    }
}

void lcd_set()
{
	struct lcd1602a_port lcd_port = {&DDRC, &DDRC, &PORTC, &PORTC};
	set_lcd_port(lcd_port);
	set_lcd_bit(4);
	lcd_init(LCD_ROWS_MAX, LCD_COLS_MAX);	
}

void lcd_item_set()
{
	int i;
	for (i = 0; i < 4; i++) {
		fnds_on(game_score);
		lcd_create_char(i, obstacle_pattern[i]);	
	}
	
	lcd_create_char(i, heart_pattern);
	lcd_create_char(i+1, player_pattern);
}

void lcd_player_set()
{	
	player_is_up = 0;
	lcd_player_draw();
}

void lcd_player_draw()
{
	lcd_move(0, player_is_up);
	lcd_putc(' ');
	lcd_move(0, !player_is_up);
	lcd_putc(PLAYER_CHAR);
}

void lcd_player_up()
{
	if (player_is_up) return;
	player_is_up = 1;
	lcd_player_draw();
}

void lcd_player_down()
{
	if (!player_is_up) return;
	player_is_up = 0;
	lcd_player_draw();
}

int lcd_create_item()
{
	lcd_move(item_x, item_y);
	lcd_putc(' ');
	item_type = rand() % 8;
	item_x = LCD_ROWS_MAX - 1;
	item_y = LCD_COLS_MAX - 1;
	
	if (item_type == 2 || item_type == 3) {
		item_y = 0;
	}
	if (item_type >= 4) {
		item_y = rand() % 2;
		item_type = 4;
	}
	
	lcd_move(item_x, item_y);
	lcd_putc(item_type);
	return 0;
}

int lcd_item_shift()
{
	if (item_type == -1)
		return -1;
	
	lcd_move(item_x, item_y);
	lcd_putc(' ');
	item_x--;
	
	lcd_move(item_x, item_y);
	lcd_putc(item_type);
	return 0;
}

int lcd_game_score()
{
	lcd_putc(game_score / 1000 + '0');
	lcd_putc((game_score / 100) % 10 + '0');
	lcd_putc((game_score / 10) % 10 + '0');
	lcd_putc(game_score % 10 + '0');
	return 0;
}

int lcd_game_grade(uint8_t game_grade, uint8_t grade_option)
{	
	switch (game_grade)
	{
		case 1:
			lcd_putc('A');
			break;
		case 2:
			lcd_putc('B');
			break;
		case 3:
			lcd_putc('C');
			break;
		case 4:
			lcd_putc('D');
			break;
		case 5:
			lcd_putc('E');
			break;
		default:
			lcd_putc('F');
		
	}
	
	switch (grade_option)
	{
		case 1:
			lcd_putc('+');
			break;
		case 2:
			lcd_putc('-');
			break;
	}
	return 0;
}

void fnds_set()
{
	DDRE = 0xFF;
	PORTE = 0x00;
	DDRB = 0xFF;
	PORTB = 0x00;
}

void fnd_on(unsigned char i, unsigned char num)
{
	const unsigned char SegAnode[10] = {
		0x03, // 0
		0x9F, // 1
		0x25, // 2
		0x0D, // 3
		0x99, // 4
		0x49, // 5
		0x41, // 6
		0x1F, // 7
		0x01, // 8
		0x09, // 9
	};
	PORTE = SegAnode[num % 10];
	PORTB &= ~(PORTB & 0x1F);
	PORTB |= (BV(i) & 0x1F);
}

void fnds_on(unsigned int num)
{
	if (num > 9999) num = 0;
	else if (num < 0) num = 9999;
	
	fnd_on(0, num / 1000);
	_delay_ms(2);
	fnd_on(1, (num / 100) % 10);
	_delay_ms(2);
	fnd_on(2, (num / 10) % 10);
	_delay_ms(2);
	fnd_on(3, num % 10);
	_delay_ms(2);
}

void fnd_game_grade(uint8_t grade)
{
	if (grade <= 0 || grade > 6) return;
	uint8_t fnd_grade_alphabet[] = {
		0x11, // A
		0x01, // B
		0x63, // C
		0x03, // D
		0x61, // E
		0x71, // F
	};
	PORTE = fnd_grade_alphabet[grade - 1];
	PORTB &= ~(PORTB & 0x1F);
	PORTB |= BV(4);
	_delay_ms(2);
}

void button_set()
{
	DDRA = 0x00;
}

void game_start()
{
	lcd_clear();
	lcd_move(0,0);
	lcd_puts("JUMPING GAME");
	lcd_move(0,1);
	lcd_puts("-START-");
	while(CHECK_SW(2))_delay_ms(1);
	while(!CHECK_SW(2))_delay_ms(1);
	while(CHECK_SW(2))_delay_ms(1);
	lcd_clear();
	lcd_player_set();
	game_set();
}

void game_finish(uint8_t is_win)
{
	uint8_t game_grade = get_game_grade();
	uint8_t grade_option = get_grade_option();
	
	lcd_clear();
	lcd_puts("GAME FINISH");
	lcd_move(0, 1);
	
	if (is_win) {
		lcd_puts("WIN - SPECIAL!");
	} else {
		lcd_puts("LOSE-");
		lcd_putc('(');
		lcd_game_score();
		lcd_putc(',');
		lcd_game_grade(game_grade, grade_option);
		lcd_putc(')');
	}
	
	while(!CHECK_SW(2))
	{
		fnds_on(game_score);
		if (!is_win) {
			fnd_game_grade(game_grade);		
		}
	}
	
	lcd_clear();
	lcd_player_set();
	game_set();
	game_start();
}

void game_set()
{
	player_is_up = 0;
	item_type = -1;
	item_x = LCD_ROWS_MAX - 1;
	item_y = 1;
	item_seed = rand() % 9425;
	item_speed = 50;
	game_score = 0;
	fnds_set();
	game_difficulty_led_set();
	game_difficulty_up();
}

void game_pause()
{
	uint8_t game_grade = get_game_grade();
	uint8_t grade_option = get_grade_option();
	
	lcd_clear();
	lcd_move(0,0);
	lcd_puts("GAME PAUSE");
	lcd_move(0,1);
	lcd_putc('(');
	lcd_game_score();
	lcd_putc(',');
	lcd_game_grade(game_grade, grade_option);
	lcd_putc(')');
	while(CHECK_SW(3))_delay_ms(1);
	while(!CHECK_SW(3)) {
		_delay_ms(1);
		if (CHECK_SW(2)) {
			game_start();
			return;
		}
	}
	while(CHECK_SW(3))_delay_ms(1);
	lcd_clear();
	lcd_player_draw();
}

void game_difficulty_led_set()
{
	DDRD = 0x3F;
	PORTD = 0x00;
	game_difficulty = 6;
}

void game_difficulty_up()
{
	game_difficulty--;
	if (game_difficulty < 0) game_difficulty = 6;
	
	PORTD |= BV(game_difficulty);
}

uint8_t get_game_grade()
{
	switch(game_score / 1000)
	{
		case 9:
		case 8:
			return 1;
		case 7:
		case 6:
			return 2;
		case 5:
		case 4:
			return 3;
		case 3:
		case 2:
			return 4;
		case 1:
			return 5;
		case 0:
			return 6;
	}
	return 6;
}

uint8_t get_grade_option()
{
	if (
	game_score > 9500 ||
	(game_score < 8000 && game_score > 7500) || 
	(game_score < 6000 && game_score > 5500) ||
	(game_score < 4000 && game_score > 3500) || 
	(game_score < 2000 && game_score > 1800) ||
	(game_score < 1000 && game_score > 800)
	) return 1;
	else if (
	(game_score < 8500 && game_score >= 8000) ||
	(game_score < 6500 && game_score >= 6000) ||
	(game_score < 4500 && game_score >= 4000) ||
	(game_score < 2500 && game_score >= 2000) ||
	(game_score < 1200 && game_score >= 1000) ||
	(game_score < 200 && game_score >= 0)
	) return 2;
	
	return 0;
}