#include <avr/io.h>
#define F_CPU 3333333UL
#include <util/delay.h>
#include "font.h"
#include "avr/interrupt.h"
#include "string.h"
#include "stdlib.h"

/*
PA7     D_PWR
PA6     SDA
PA5     SCL
PA4     CS
PA3     D/C
PA2     RST
PA1     BSY

PC0     DCF_SIG
PC1     DCF_PWR 

PC3     LED

PB4     U_BAT

*/

void write_byte(uint8_t data);
void send_data(uint8_t data);
void send_cmd(uint8_t cmd);
void sda(uint8_t val);
void scl(uint8_t val);
void update();
void init();
void standby();
void write_char_at(char c, uint8_t x, uint8_t y);
void write_string_at(char* string, uint8_t string_length, uint8_t x, uint8_t y);
void number_to_string(int number, char *result, uint8_t result_length);
void setX(uint8_t x);
void setY(uint8_t y);

bool get_data;
void write_72_at(uint8_t number, uint8_t x, uint8_t y);

int main() {
    init();
    
    char test[20];
      
    write_72_at(0, 0, 0);
    write_72_at(1, 1, 0);
    write_72_at(2, 2, 0);
    write_72_at(3, 3, 0);    

    update();
    standby();
    PORTB_OUTSET = PIN4_bm;
    _delay_ms(1000);
    while (1) {
        PORTB_OUTSET = PIN4_bm;
        _delay_ms(1000);
        PORTB_OUTCLR = PIN4_bm;
        _delay_ms(1000);
    }
    return 0;
}

void sda(uint8_t val) {
    if (val) 
        PORTA_OUTSET = PIN6_bm;
    else
        PORTA_OUTCLR = PIN6_bm;
}
void scl(uint8_t val) {
    if (val) 
        PORTA_OUTSET = PIN5_bm;
    else
        PORTA_OUTCLR = PIN5_bm;
}

void write_byte(uint8_t data) {
    PORTA_OUTCLR = PIN4_bm; //cs 0
    for(int i = 0; i < 8; i++) {
        sda(data & 0x80);
        _delay_us(10);
        scl(1);
        _delay_us(10);
        scl(0);
        data = data << 1;
    }
    PORTA_OUTSET = PIN4_bm; //cs 1
   
}
void send_cmd(uint8_t cmd) {
    PORTA_OUTCLR = PIN3_bm; //dc 0
    write_byte(cmd);
}
void send_data(uint8_t data) {
    PORTA_OUTSET = PIN3_bm; //dc 1
    write_byte(data);
}

void init() {
    PORTA_DIRSET = PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm | PIN6_bm | PIN7_bm;
    _delay_ms(50);
    PORTA_OUTCLR = PIN2_bm; //rst 0
    _delay_ms(10);
    PORTA_OUTSET = PIN2_bm; //rst high

    _delay_ms(10);
  //  PORTB_OUTCLR = PIN3_bm; //scl low ?
    
    send_cmd(0x12);
    _delay_ms(20);

    send_cmd(0x01);
    send_data(0xc7);
    send_data(0);
    send_data(0);
    
    send_cmd(0x11);
    send_data(1);

    send_cmd(0x44);
    send_data(0x00);
    send_data(0x18);

    send_cmd(0x45);
    send_data(0xC7);
    send_data(0);
    send_data(0);
    send_data(0);

    send_cmd(0x1A);
    send_data(0);
    
    send_cmd(0x22);   
    send_data(0xb1);

    send_cmd(0x20);
    while (PORTA_IN & PIN1_bm) //bsy
        _delay_ms(10);
    
    send_cmd(0x4e);
    send_data(0x00);

    send_cmd(0x4F);
    send_data(0xC7);
    send_data(0);

    send_cmd(0x24);
    for(int i = 0; i < 5000; i ++) {
        send_data(0xFF);
    }
    update();
}

void update() {
    send_cmd(0x4e);
    send_data(0x00);

    send_cmd(0x4F);
    send_data(0xC7);
    send_data(0);

    send_cmd(0x22);
    send_data(0xc7);

    send_cmd(0x20);
    while (PORTA_IN & PIN1_bm)
        _delay_ms(10);
}

void standby() {
    send_cmd(0x10);
    send_data(0x01);
    PORTA_OUTCLR = PIN7_bm;
}

void write_char_at(char c, uint8_t x, uint8_t y) {
    for(int i = 0; i < 10; i ++) {
        setY(2 * y);
        setX(i + 10 * x);
        send_cmd(0x24);        
        send_data(~font[c][i*2+1]);
        send_data(~font[c][i*2]);         
    }
}
void write_string_at(char* string, uint8_t string_length, uint8_t x, uint8_t y) {
    if (x + string_length > 20) {
        return;
    }
    for (int i = 0; i < string_length; i++) {
        write_char_at(string[i], x + i, y);
    } 
}

void setX(uint8_t x) {
    send_cmd(0x4f);
    send_data(199 - x);
    send_data(0);
}
void setY(uint8_t y) {
    send_cmd(0x4e);
    send_data(y);
}

uint16_t scale_byte(uint8_t in) {
    uint16_t res = 0;
    for (int i = 0; i < 8; i++) {
        if (in & 1 << i) {
            res |= 1 << (i*2) | 1 << (i*2 + 1);    
        }      
    }
    return res;
}

void write_large_char_at(char c, uint8_t x, uint8_t y) {
    uint16_t lsb, msb;
    for(int i = 0; i < 10; i ++) {
        lsb = scale_byte(~font[c][i*2+1]);
        msb = scale_byte(~font[c][i*2]);
        setY(y);
        setX(2 * i + 11 * x);
        send_cmd(0x24);        
        send_data(lsb>>8);
        send_data(lsb & 0xFF);
        send_data(msb>>8);         
        send_data(msb & 0xFF); 
        setY(y);  
        setX(2 * i + 11 * x + 1);
        send_cmd(0x24);        
        send_data(lsb>>8);
        send_data(lsb & 0xFF);
        send_data(msb>>8);         
        send_data(msb & 0xFF);        
    }
}

void number_to_string(int number, char *result, uint8_t result_length) {
    if (result_length < 6) {
        return;
    }
    uint8_t index;
    if (number < 0) {
        result[index++] = '-';
        number = number * -1;
    }
    else {
        result[index++] = ' ';
    }
    int teiler = 10000;
    while (number > 0) {
        result[index++] = (number / teiler) + '0';
        number = number % teiler;
        teiler /= 10;
    }
}

void write_72_at(uint8_t number, uint8_t x, uint8_t y) {
    if (number > 9 )
        return;
    for (int i = 0; i < 48; i++) {
        setX(48 * x + i);
        setY(9 * y);
        send_cmd(0x24);
        for (int j = 0; j < 9; j++) {
            
            send_data(~calibri_72ptBitmaps[number * 432 + j + 9 * i]);
        }
    }
}
