#include <LPC214x.H>

#define MASK_P12 (1 << 12) // mask for P0.12
#define MASK_P21 (1 << 21) // mask for P0.21
#define MASK_P17 (1 << 17) // mask for P0.17
#define MASK_P18 (1 << 18) // mask for P0.18
#define MASK_P19 (1 << 19) // mask for P0.19
#define MASK_P20 (1 << 20) // mask for P0.20

// String functioning
#define clearDisplay 0x01
#define homeDC 0x02
#define incrementNoShift 0x06
#define cursorUB 0x0F
#define operationMode 0x3C
#define secondLine 0xC0

void init_adc(void);
unsigned int read_adc(void);
unsigned int adc_to_delay(unsigned int adc_value);
void display_speed_percentage(unsigned int adc_value);

void delay_ms(unsigned int ms) { // Debounce delay
    unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 1000; j++) {
        }
    }
}

// LCD Screen Functions
void enable() {
    int i;
    for (i = 0; i < 1000; i++);
    
    IOCLR1 |= 1 << 25;
    
    for (i = 0; i < 1000; i++);
    
    IOSET1 |= 1 << 25;
}

void sendCommand(int command) {
    IOCLR1 |= 1 << 24;
    IOCLR1 |= 0xFF0000;
    IOSET1 |= command << 16;
    enable();
    delay_ms(2); // Adding a small delay after sending a command
}

void sendData(int data) {
    IOSET1 |= 1 << 24;
    IOCLR1 |= 0xFF0000;
    IOSET1 |= data << 16;
    enable();
    delay_ms(2); // Adding a small delay after sending data
}

void setCursorPosition(int position) {
    sendCommand(position);
}

void configGpios() {
    IODIR0 |= 1 << 30;
    IOSET0 = 1 << 30;
    
    IODIR0 |= 1 << 22;
    IOCLR0 |= 1 << 22;
    
    IODIR1 |= 0x3FF0000;
    
    IOSET1 |= 1 << 25;
}

void sendString(char *text) {
    int i = 0;
    while (text[i] != '\0') {
        sendData(text[i]);
        i++;
    }
}

// ADC initialization
void init_adc(void) {
    // Settings for ADC0 Interrupt
    VICVectCntl0 = 0x32;
    VICIntEnable |= 0x40000;

    AD0CR = 0x00217804;  // Select AD0.2 (P0.29) and configure ADC
    AD0CR |= 0x00010000;

    PINSEL1 &= ~0x0F000000; // Clear bits for P0.28 and P0.29
    PINSEL1 |= 0x05000000; // Select AD0.2 function for P0.29
}

// Function to read ADC value
unsigned int read_adc(void) {
    unsigned int adc_value;

    AD0CR |= 0x01000000; // Start conversion
    while ((AD0GDR & 0x80000000) == 0); // Wait for conversion to complete
    adc_value = (AD0GDR >> 6) & 0x3FF; // Get the 10-bit ADC result

    return adc_value;
}

// Function to convert ADC value to delay in ms
unsigned int adc_to_delay(unsigned int adc_value) {
    unsigned int min_delay = 15; // Minimum delay in ms
    unsigned int max_delay = 75; // Maximum delay in ms

    // Linear mapping from ADC value to delay
    float normalized_value = (float)adc_value / 1023.0;
    unsigned int delay = (unsigned int)(min_delay + (max_delay - min_delay) * normalized_value);

    return delay;
}

void display_speed_percentage(unsigned int adc_value) {
    unsigned int speed_percentage = 100-((adc_value * 100) / 1023);
    char buffer[16];
    sprintf(buffer, "Speed: %3u%%", speed_percentage);
    setCursorPosition(secondLine);
    sendString(buffer);
}

void motor_forward(unsigned int delay_time) {
    // Step 1: P0.12 high, P0.21 low
    IO0SET = MASK_P12; // Set P0.12
    IO0CLR = MASK_P21; // Clear P0.21
    delay_ms(delay_time);

    // Step 2: Both low
    IO0CLR = MASK_P12; // Clear P0.12
    IO0CLR = MASK_P21; // Clear P0.21
    delay_ms(delay_time);

    // Step 3: P0.12 low, P0.21 high
    IO0CLR = MASK_P12; // Clear P0.12
    IO0SET = MASK_P21; // Set P0.21
    delay_ms(delay_time);

    // Step 4: Both high
    IO0SET = MASK_P12; // Set P0.12
    IO0SET = MASK_P21; // Set P0.21
    delay_ms(delay_time);
}

void motor_backward(unsigned int delay_time) {
    // Step 1: P0.12 low, P0.21 high
    IO0CLR = MASK_P12; // Clear P0.12
    IO0SET = MASK_P21; // Set P0.21
    delay_ms(delay_time);

    // Step 2: Both low
    IO0CLR = MASK_P12; // Clear P0.12
    IO0CLR = MASK_P21; // Clear P0.21
    delay_ms(delay_time);

    // Step 3: P0.12 high, P0.21 low
    IO0SET = MASK_P12; // Set P0.12
    IO0CLR = MASK_P21; // Clear P0.21
    delay_ms(delay_time);

    // Step 4: Both high
    IO0SET = MASK_P12; // Set P0.12
    IO0SET = MASK_P21; // Set P0.21
    delay_ms(delay_time);
}

int main() {
    unsigned int adc_value, delay_time;
    int prev_state = 0; // Previous state: 0 - None, 1 - Forward, 2 - Backward

    // LCD Screen configuration
    configGpios();
    sendCommand(operationMode);
    sendCommand(cursorUB);
    sendCommand(incrementNoShift);
    sendCommand(clearDisplay);
    sendCommand(homeDC);
    
    init_adc(); // Initialize ADC

    // Configure P0.12 and P0.21 as GPIO output
    IO0DIR |= MASK_P12; // Set P0.12 as output
    IO0DIR |= MASK_P21; // Set P0.21 as output
    IO0DIR &= ~(MASK_P17 | MASK_P18 | MASK_P19 | MASK_P20); // Set P0.17, P0.18, P0.19, and P0.20 as input

    while (1) {
        // Check if P0.19 and P0.20 are low (backward)
        if ((IO0PIN & MASK_P19) == 0 && (IO0PIN & MASK_P20) == 0) {
            if (prev_state != 1) {
                sendCommand(clearDisplay);
                sendString(" Backward");
                prev_state = 1;
            }
            adc_value = read_adc(); // Read ADC value
            delay_time = adc_to_delay(adc_value); // Convert ADC value to delay
            display_speed_percentage(adc_value);
            motor_backward(delay_time);
        // Check if P0.17 and P0.18 are low (forward)
        } else if ((IO0PIN & MASK_P17) == 0 && (IO0PIN & MASK_P18) == 0) {
            if (prev_state != 2) {
                sendCommand(clearDisplay);
                sendString(" Forward");
                prev_state = 2;
            }
            adc_value = read_adc(); // Read ADC value
            delay_time = adc_to_delay(adc_value); // Convert ADC value to delay
            display_speed_percentage(adc_value);
            motor_forward(delay_time);
        } else {
            if (prev_state != 0) {
                sendCommand(clearDisplay);
                sendString(" Stopped");
                prev_state = 0;
            }
						adc_value = read_adc(); // Read ADC value
            delay_time = adc_to_delay(adc_value); // Convert ADC value to delay
            display_speed_percentage(adc_value);
            
        }
    }
}
