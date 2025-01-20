#include <avr/io.h>
#define F_CPU 3333333UL
#include <util/delay.h>
#include "font.h"
#include "avr/interrupt.h"
#include "string.h"

/*
PB2     SDA
PB3     SCL
PB1     CS
PB0     D/C
PC2     RST
PC3     BSY

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
//uint16_t scale_byte(uint8_t in);
//void write_large_char_at(char c, uint8_t x, uint8_t y);

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
        PORTB_OUTSET = PIN2_bm;
    else
        PORTB_OUTCLR = PIN2_bm;
}
void scl(uint8_t val) {
    if (val) 
        PORTB_OUTSET = PIN3_bm;
    else
        PORTB_OUTCLR = PIN3_bm;
}
void write_byte(uint8_t data) {
    PORTB_OUTCLR = PIN1_bm;
    for(int i = 0; i < 8; i++) {
        sda(data & 0x80);
        _delay_us(10);
        scl(1);
        _delay_us(10);
        scl(0);
        data = data << 1;
    }
    PORTB_OUTSET = PIN1_bm;
   
}
void send_cmd(uint8_t cmd) {
    PORTB_OUTCLR = PIN0_bm;
    write_byte(cmd);
}
void send_data(uint8_t data) {
    PORTB_OUTSET = PIN0_bm;
    write_byte(data);
}

void init() {

    PORTB_DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm;
    PORTC_OUTSET = PIN2_bm;
    PORTB_OUTSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
    _delay_ms(50);
   // PORTB_OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
    PORTC_DIRSET = PIN2_bm;
    PORTC_OUTCLR = PIN2_bm;
    _delay_ms(10);
    PORTC_OUTSET = PIN2_bm;
    _delay_ms(10);
    PORTB_OUTCLR = PIN3_bm;
    
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
    while (PORTC_IN & PIN3_bm)
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
    while (PORTC_IN & PIN3_bm)
        _delay_ms(10);
}

void standby() {
    send_cmd(0x10);
    send_data(0x01);
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
