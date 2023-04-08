#include <stdint.h>
#include <stdlib.h>
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

// Mode button: B6.

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
    MODE_DIRECT = 0,
    MODE_VU,
    MODE_ATTRACT,
    MODE_MAX, // for cycling
} Mode;

// Timer 1 is used to refresh the LED pattern from the disply buffer(dbuf).

// Mode summary:
// DIRECT mode: samples analog 0 input @122 Hz on Timer 0 and updates dbuf.
// VU mode: direct mode scaled up by x16 ;)
// ATTRACT mode: displays a series of pretty patterns. Uses Timer 0 @122 Hz
// for each frame.

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

void init_button(void) {
    DDRB &= ~(BV(6));
    PORTB |= BV(6);
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

void enter_mode(Mode new_mode) {
    mode = new_mode;
    // set up/shut down ADC for each mode.
    switch (mode) {
    case MODE_VU:
        // Set the ADC to sample continuously.
        // ADCSRB: ADC free running (continuous), ADTS[2:0] = 000
        ADCSRB = 0;
        // ADCSRA: ADC is enabled, autotriggers, starts conversion, prescaler /8
        ADCSRA = BV(ADEN) | BV(ADSC) | BV(ADATE) | BV(ADIE) | BV(ADPS2) | BV(ADPS1);
        break;
    case MODE_DIRECT:
        // Set the ADC to sample on timer 0 (122 Hz).
        // ADCSRB: ADC auto trigger on Timer0 overflow, ADTS[2:0] = 100
        ADCSRB = BV(ADTS2);
        // ADCSRA: ADC is enabled, autotriggers, starts conversion, prescaler /8
        ADCSRA = BV(ADEN) | BV(ADSC) | BV(ADATE) | BV(ADIE) | BV(ADPS2) | BV(ADPS1);
        break;
    case MODE_ATTRACT:
    default: // bad values -> attract
        // Shut down the ADC; it is not used.
        ADCSRA = 0;
        break;
    }
}

// Initialize the static parameters on the adc 
void init_adc(void) {
    // ADMUX: Reference voltage is AVcc, selected with REFS[1:0] = 01
    // ADMUX: sampling ADC0 with MUX[3:0] = 0000
    ADMUX = BV(REFS0);
    // DIDR0: Disable the digital input on the pin we're using to sample analog
    // to save a tiny bit of power. Pretty deep cut.
    DIDR0 = BV(ADC0D);
    
}

uint16_t build_pat(uint8_t v) {
    uint16_t rv = 0;
    for (int8_t i = 0; i < 16; i++) {
        rv = rv << 1;
        if (dbuf[15-i] > v) rv |= 0x01;
    }
    return rv;
}

int main (void) {
    init_display();
    init_button();
    // PC1 is "analog 1", which we'll use for our button
    PORTC |= BV(1); // Set the pull-up
    init_timers();
    init_adc();
    enter_mode(MODE_ATTRACT);
    sei();
    for (int i = 0; i < 16; i++) {
        dbuf[i] = gamma_table[i];
    }
    while(1) {
    };
    return (0);
}

static uint16_t ctr = 0;

static uint8_t button_timeout = 0; // We use this to debounce pushes on PC1


// Timer 0 triggers at 122Hz.

ISR(TIMER0_OVF_vect) {
    if ((PINB & BV(6)) == 0) {
        if (button_timeout == 0) {
            button_timeout = 16; // debounce
            // Perform press
            enter_mode((mode + 1) % MODE_MAX);
        }
    } else { if (button_timeout) button_timeout--; }
    if (mode == MODE_ATTRACT) {
        ctr += 323;
        int16_t pulse = (ctr >> 4);
        for (int8_t i = 0; i < 16; i++) {
            // Slow pulse: find distance from center.
            int16_t mark = i << 8;
            int16_t distance = 255 - abs(mark - pulse);
            dbuf[i] = (distance>0)?distance:0;
        }
    }
}

uint8_t slice = 0;

ISR(TIMER1_OVF_vect) {
    if (mode == MODE_ATTRACT) {
        slice += 16;
        display_bits(build_pat(slice));
    }
}


ISR(ADC_vect) {
    uint16_t v;
    v = ADCL;  
    v += (ADCH<<8); // do we need to prevent out-of-order here?
    switch (mode) {
    case MODE_DIRECT:
        v = (v >> 4);
    case MODE_VU:
        v = (v >> 2) & 0x0f;
        uint16_t pat = 0;
        while (v--) {
            pat = (pat<<1) | 1;
        }
        display_bits(pat);
        break;
    default:
        break;
    }
}

