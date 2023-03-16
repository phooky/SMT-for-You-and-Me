#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BV(x) (1 << x)
// bit mask and shift 
#define BMS(a,from,to) (((a >> from) & 0x01) << to)

// The display buffer. Each element has 8 brightness levels.
uint8_t dbuf[16];

// Timer interrupts:
// The control timer (Timer 0): ~100Hz
// The display timer (Timer 1): ~4KHz

// Pattern from left to right is:
// 0:  D2, C5, C4, C3,
// 4:  C2, E3, E2, B1,
// 8:  B0, D7, D6, D5,
// C:  E1, E0, D4, D3

// The bit patterns for each port.
const uint8_t bpat = BV(0) | BV(1);
const uint8_t cpat = BV(2) | BV(3) | BV(4) | BV(5);
const uint8_t dpat = BV(2) | BV(3) | BV(4) | BV(5) | BV(6) | BV(7);
const uint8_t epat = BV(0) | BV(1) | BV(2) | BV(3);

const uint8_t gamma_table[16] = {
    0, 1, 4, 9, 16, 25, 36, 49,
    64, 81, 100, 121, 144, 169, 196, 255
};

typedef enum {
    MODE_TEST = 0,
    MODE_VU,
    MODE_ATTRACT,
} Mode;

static Mode mode = MODE_VU;

void display_bits(uint16_t pattern) {
    pattern = ~pattern;
    PORTB = (PORTB & ~bpat) |
        BMS(pattern, 7, 1) |
        BMS(pattern, 8, 0);
    PORTC = (PORTC & ~cpat) |
        BMS(pattern, 1, 5) |
        BMS(pattern, 2, 4) |
        BMS(pattern, 3, 3) |
        BMS(pattern, 4, 2);
    PORTD = (PORTD & ~dpat) |
        BMS(pattern, 0, 2) |
        BMS(pattern, 9, 7) |
        BMS(pattern, 10, 6) |
        BMS(pattern, 11, 5) |
        BMS(pattern, 14, 4) |
        BMS(pattern, 15, 3);
    PORTE = (PORTE & ~epat) |
        BMS(pattern, 5, 3) |
        BMS(pattern, 6, 2) |
        BMS(pattern, 12, 1) |
        BMS(pattern, 13, 0);
}

void init_display(void) {
    PORTB = bpat; DDRB = bpat;
    PORTC = cpat; DDRC = cpat;
    PORTD = dpat; DDRD = dpat;
    PORTE = epat; DDRE = epat;
}
    
void init_timers(void) {
    // Timer 0: ~122 Hz
    // Timer 0: WGM mode 0 (overflow at 0xff)
    // Timer 0: CS mode 4 (/256)
    // Timer 0: This gives us a frequency of about 122 Hz.
    TCCR0A = 0x00;
    TCCR0B = 0x04;
    TIMSK0 = 0x01; // interrupt on overflow

    // Timer 1: ~4KHz (3.90625 kHz)
    // Timer 1: WGM mode 5 (overflow at 0xff)
    // Timer 1: CS mode 2 (/8)
    TCCR1A = 0x01;
    TCCR1B = 0x0A;
    TIMSK1 = 0x01; // interrupt on overflow
}

// Start the adc 
void start_adc(void) {
    // ADMUX: Reference voltage is AVcc, selected with REFS[1:0] = 01
    // ADMUX: sampling ADC0 with MUX[3:0] = 0000
    ADMUX = BV(REFS0);
    // ADCSRB: ADC auto trigger on Timer0 overflow, ADTS[2:0] = 100
    ADCSRB = BV(ADTS2);
    // ADCSRA:
    ADCSRA = BV(ADEN) | BV(ADSC) | BV(ADATE) | BV(ADIE) | BV(ADPS2) | BV(ADPS1);
}

uint16_t build_pat(uint8_t v) {
    uint16_t rv = 0;
    for (int8_t i = 0; i < 16; i++) {
        if (dbuf[15-i] >= v) rv |= 0x01;
        rv = rv << 1;
    }
    return rv;
}

int main (void) {
    init_display();
    start_adc();
    sei();
    for (int i = 0; i < 16; i++) {
        dbuf[i] = gamma_table[i];
    }
    init_timers();
    while(1) {
        switch(mode) {
        case MODE_VU:
            
            break;
        }
    };
    return (0);
}

static uint8_t ctr = 0;
static uint8_t div = 0;

ISR(TIMER0_OVF_vect) {
    if (div == 0) {
        //display_bits(1 << (ctr %16));
        ctr++;
    }
    div = (div+1)%20;
}

static uint8_t slice = 0;

ISR(TIMER1_OVF_vect) {
    slice += 16;
    //display_bits(build_pat(slice));
}

ISR(ADC_vect) {
    uint16_t v;
    v = ADCL;  
    v += (ADCH<<8); // do we need to prevent out-of-order here?
    v = (v >> 6) & 0x0f; // scale to 0-16
    uint16_t pat = 0;
    while (v--) {
        pat = (pat<<1) | 1;
    }
    display_bits(pat);
}

