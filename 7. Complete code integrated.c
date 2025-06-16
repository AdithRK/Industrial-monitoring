#include "stm32f405xx.h" 

#include "cmn.h" 

#include "lcd.h" 

 

#include <stdint.h> 

#include <stdio.h> 

#include <string.h> 

 

// === Passkey Definitions === 

char passkey[5] = "1234"; 

char entered[5]; 

int key_index = 0; 

const char keymap[4][4] = { 

    {'1', '2', '3', 'A'}, 

    {'4', '5', '6', 'B'}, 

    {'7', '8', '9', 'C'}, 

    {'*', '0', '#', 'D'} 

}; 

 

// === Sensor/Motor Definitions === 

#define AIN1_PIN    12   // PA12 - Motor control 

 

#define AIN2_PIN    4   // PA4 - Motor control 

#define BIN1_PIN      3   // PA3 - Motor 2 

#define BIN2_PIN      2   // PA2 - Motor 2 

#define TEMP_ADC_CH   5   // PA5 = ADC1_IN5 

#define LDR_PIN     11   // PA11 - LDR input 

 

#define LED_PIN     6   // PA6 - LED output 

#define LED1_PIN     6 

#define LED2_PIN     7 

#define BUZZER_PIN  7   // PA7 - Buzzer output 

 

// === Global Variables === 

volatile uint16_t adc_value = 0; 

volatile float voltage = 0.0; 

volatile int temperature = 0.0; 

volatile float t1 = 0.0; 

uint16_t result=0; 

// Function prototypes 

 

extern void delayms(uint32_t ms); 

 

 

// === Function Prototypes === 

void Keypad_GPIO_Init(void); 

char Keypad_Scan(void); 

void gpio_init(void); 

void motor_on(void); 

void motor_off(void); 

void motor2_on(void); 

void motor2_off(void); 

void toggle_pin(uint8_t pin); 

void adc_init(void); 

uint16_t adc_read(void); 

void adc2_init(void); 

uint16_t adc2_read(void); 

void led_init(void); 

void led_control(uint16_t); 

 

 

void gpio_init(void) { 

 

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock 

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 

    // PA11 as Input (LDR) 

 

    GPIOA->MODER &= ~(3 << (LDR_PIN * 2)); 

 

    GPIOA->PUPDR &= ~(3 << (LDR_PIN * 2));  // No pull 

 

    // PA4, PA5 as Output (Motor) 

 

    GPIOA->MODER &= ~((3 << (AIN1_PIN * 2)) | (3 << (AIN2_PIN * 2))); 

 

    GPIOA->MODER |= (1 << (AIN1_PIN * 2)) | (1 << (AIN2_PIN * 2)); 

 

    GPIOA->OTYPER &= ~((1 << AIN1_PIN) | (1 << AIN2_PIN)); 

 

    GPIOA->OSPEEDR |= (3 << (AIN1_PIN * 2)) | (3 << (AIN2_PIN * 2)); 

 

    // PA2, PA3 as Output (Motor) 

 

        GPIOA->MODER &= ~((3 << (BIN1_PIN * 2)) | (3 << (BIN2_PIN * 2))); 

 

        GPIOA->MODER |= (1 << (BIN1_PIN * 2)) | (1 << (BIN2_PIN * 2)); 

 

        GPIOA->OTYPER &= ~((1 << BIN1_PIN) | (1 << BIN2_PIN)); 

 

        GPIOA->OSPEEDR |= (3 << (BIN1_PIN * 2)) | (3 << (BIN2_PIN * 2)); 

 

 

    // PA6 (LED) and PA7 (Buzzer) as Output 

 

    GPIOA->MODER &= ~((3 << (LED_PIN * 2)) | (3 << (BUZZER_PIN * 2))); 

 

    GPIOA->MODER |= (1 << (LED_PIN * 2)) | (1 << (BUZZER_PIN * 2)); 

 

    GPIOA->OTYPER &= ~((1 << LED_PIN) | (1 << BUZZER_PIN)); 

 

    GPIOA->OSPEEDR |= (3 << (LED_PIN * 2)) | (3 << (BUZZER_PIN * 2)); 

    // 2. Set PC6 and PC7 as General Purpose Output Mode (MODER = 01) 

        GPIOC->MODER &= ~((3 << (2 * LED1_PIN)) | (3U << (2 * LED2_PIN)));  // Clear mode bits 

        GPIOC->MODER |=  (1 << (2 * LED1_PIN)) | (1U << (2 * LED2_PIN));    // Set as output (01) 

        GPIOC->MODER &= ~(3 << (2 * 9));   // Clear mode 

            GPIOC->MODER |=  (1 << (2 * 9));   // Set to 01 (output) 

 

 

 

    } 

 

 

 

// Motor Control 

 

void motor_on(void) { 

 

    GPIOA->ODR |= (1 << AIN1_PIN); 

 

    GPIOA->ODR &= ~(1 << AIN2_PIN); 

 

} 

 

void motor_off(void) { 

 

    GPIOA->ODR &= ~(1 << AIN1_PIN); 

 

    GPIOA->ODR &= ~(1 << AIN2_PIN); 

 

} 

// === Motor 2 Control === 

void motor2_on(void) { 

    GPIOA->ODR |=  (1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

void motor2_off(void) { 

    GPIOA->ODR &= ~(1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

 

 

 

// Toggle GPIO pin 

 

void toggle_pin(uint8_t pin) { 

 

    GPIOA->ODR |= (1 << pin);  // XOR toggles bit 

 

} 

 

 

// === ADC Initialization === 

void adc_init(void) { 

    RCC->AHB1ENR |= (1 << 0);               // Enable GPIOA clock 

    GPIOA->MODER |= (3 << (5 * 2));         // PA5 -> Analog mode 

 

    RCC->APB2ENR |= (1 << 8);               // Enable ADC1 clock 

    ADC1->SQR3 = 5;                         // Channel 5 (PA5 = ADC1_IN5) 

 

    ADC1->CR1 |= (1 << 8);                  // Scan mode (not needed for 1 ch but ok) 

    ADC1->CR2 |= (1 << 1);                  // Continuous conversion 

    ADC1->CR2 |= (1 << 0);                  // Enable ADC 

} 

 

// === ADC Read === 

uint16_t adc_read(void) { 

    ADC1->CR2 |= (1 << 30);                 // Start conversion 

    while (!(ADC1->SR & (1 << 1)));         // Wait for end of conversion 

    return ADC1->DR; 

} 

 

 

void adc2_init(void) 

{ 

    RCC->AHB1ENR |= (1 << 2);      // Enable GPIOC clock 

 

    // Set PC2 to analog mode 

    GPIOC->MODER |= (3 << (2 * 2));  // PC2 = analog mode (MODER2[1:0] = 11) 

 

    RCC->APB2ENR |= (1 << 9);      // Enable ADC2 clock 

 

    ADC2->SQR3 = 12;               // Select channel 12 (PC2) as first in regular sequence 

    ADC2->CR1 = 0;                 // Disable scan mode (only 1 channel needed) 

    ADC2->CR2 |= (1 << 1);         // Enable continuous conversion 

    ADC2->CR2 |= (1 << 0);         // Enable ADC2 

} 

 

// === ADC2 Read Function === 

uint16_t adc2_read(void) 

{ 

    ADC2->CR2 |= (1 << 30);                  // Start conversion 

    while (!(ADC2->SR & (1 << 1)));          // Wait for EOC 

    return ADC2->DR;                         // Return ADC result 

} 

 

// LED Initialization for PC6, PB13, PB14, PB15 

 

void led_init(void) 

 

{ 

 

    RCC->AHB1ENR |= (1 << 2);  // Enable GPIO Clock for PortC 

 

 

 

    // Set PC0–PC5 as output 

 

        for (int i = 0; i <= 5; i++) 

 

            GPIOC->MODER |= (1 << (i * 2)); 

 

 

} 

 

 

void led_control(uint16_t adc_value) 

{ 

    // Turn OFF all LEDs 

    GPIOC->ODR &= ~0x3F; // Clear PC0–PC5 (bits 0–5) 

 

 

 

 

    int index = adc_value / 410;  // Get range 0–9 

 

    switch (index) 

    { 

        case 9: GPIOC->ODR |= (1 << 0); 

        case 8: GPIOC->ODR |= (1 << 0); 

        case 7: GPIOC->ODR |= (1 << 1); 

        case 6: GPIOC->ODR |= (1 << 1); 

        case 5: GPIOC->ODR |= (1 << 3); 

        case 4: GPIOC->ODR |= (1 << 3); 

        case 3: GPIOC->ODR |= (1 << 4); 

        case 2: GPIOC->ODR |= (1 << 4); 

        case 1: GPIOC->ODR |= (1 << 5); 

        case 0: GPIOC->ODR |= (1 << 5); 

 

        default: break; 

    } 

} 

 

 

// === Passkey Entry === 

void wait_for_passkey(void) { 

 

Keypad_GPIO_Init(); 

 

    lprint(0x80, "Welcome"); 

 

    do 

 

        { 

 

            char key = Keypad_Scan(); 

 

            if (key != 0) 

 

              { 

 

                 if (key == '*') 

 

                   { 

 

 

    			 key_index = 0; 

 

    			 for (int i = 0; i < 5; i++) 

 

    			 entered[i] = 0; 

 

    			 lprint(0x80, "Input Your Passkey"); 

 

    			 lprint(0xC0, "                "); 

 

                        } 

 

                        else if (key == '#' && key_index > 0) 

 

                        { 

 

                            entered[key_index] = '\0'; 

 

                            if (strcmp(entered, passkey) == 0) 

 

                            { 

 

                                lprint(0x80, "Please Come In   "); 

 

                                lprint(0xC0, "                "); 

 

                                GPIOC->ODR |= (1 << 1);     // Green LED ON 

 

                                GPIOC->ODR &= ~(1 << 0);    // Red LED OFF 

 

                                delayms(500); 

 

                                break; // Exit loop on success 

 

                            } 

 

                            else 

 

                            { 

 

                                lprint(0x80, "Wrong Passkey!   "); 

 

                                lprint(0xC0, "Try Again!       "); 

 

                                GPIOC->ODR |= (1 << 0);     // Red LED ON 

 

                                GPIOC->ODR &= ~(1 << 1);    // Green LED OFF 

 

                                delayms(250); 

 

                                key_index = 0; 

 

                                memset(entered, 0, sizeof(entered)); 

 

                                LcdFxn(0,0x01); 

 

                                lprint(0x80, "Input Your Passkey"); 

 

                            } 

 

                        } 

 

                        else if (key_index < 4) 

 

                        { 

 

                            entered[key_index++] = key; 

 

                            char buf[17] = "Entered: "; 

 

    //                        for (int i = 0; i < key_index; i++) 

 

    //                            buf[9 + i] = '*'; 

 

                            strncpy(&buf[9], entered, key_index); 

 

                            buf[9 + key_index] = '\0'; 

 

                            lprint(0xC0, buf); 

 

                        } 

 

                    } 

 

            } while (1); // Repeat until correct passkey 

        } 

 

 

 

 

// === MAIN === 

int main(void) { 

    LcdInit();             // LCD Init first 

    RCC->AHB1ENR |= (1 << 2);  // Enable GPIOC for status LEDs 

    GPIOC->MODER |= (1 << 20) | (1 << 22); // PC10, PC11 Output 

    GPIOC->ODR &= ~((1 << 10) | (1 << 11)); // Clear 

 

    wait_for_passkey();    // Block until correct passkey entered 

 

    gpio_init(); 

    adc_init(); 

    adc2_init(); 

    led_init(); 

 

    char buffer[17]; 

 

    while (1) { 

        result = adc2_read(); 

        int gas_percent = (result * 100) / 4095; 

 

        adc_value = adc_read(); 

        voltage = (adc_value / 4095.0) * 3.3; 

        t1 = -50.0 * voltage; 

        temperature = t1 + 104.5; 

 

        char temp_str[17]; 

        sprintf(temp_str, "Temp: %d C", temperature); 

        lprint(0x80, temp_str); 

 

        if (temperature > 27.0) { 

            motor2_on(); 

            lprint(0xC0, "Smoke Detected   "); 

            GPIOC->ODR |= (1 << 9); 

            GPIOC->ODR |= (1 << LED1_PIN) ; 

        } else { 

            motor2_off(); 

            lprint(0xC0, "                   "); 

            GPIOC->ODR &= ~(1 << 9); 

            GPIOC->ODR &= ~(1 << LED1_PIN) ; 

        } 

 

        if (GPIOA->IDR & (1 << LDR_PIN)) { 

            motor_on(); 

            toggle_pin(LED_PIN); 

            toggle_pin(BUZZER_PIN); 

            lprint(0xC0, "Fire Detected    "); 

        } else { 

            motor_off(); 

            GPIOA->ODR &= ~(1 << LED_PIN); 

            GPIOA->ODR &= ~(1 << BUZZER_PIN); 

            lprint(0xC0, "                   "); 

        } 

 

        if (gas_percent > 20) { 

            sprintf(buffer, "Gas: %3d%%       ", gas_percent); 

            lprint(0xC0, buffer); 

        } 

 

        if ((temperature < 27.0) && 

            !(GPIOA->IDR & (1 << LDR_PIN)) &&  // LDR not detecting 

            (gas_percent < 20)) { 

 

            GPIOC->ODR |= (1 << LED2_PIN);  // Turn ON PC7 

            lprint(0xC0, "safe    "); 

        } else { 

            GPIOC->ODR &= ~(1 << LED2_PIN); // Turn OFF PC7 

            lprint(0xC0, "                     "); 

        } 

 

        led_control(result); 

        delayms(100); 

    } 

} 

 

// === GPIO INIT (Sensors + Motors) === 

 

// === Keypad GPIO Setup === 

void Keypad_GPIO_Init(void) { 

    RCC->AHB1ENR |= (1 << 1); // GPIOB 

    // Rows PB0–PB3 output 

    for (int i = 0; i < 4; i++) { 

        GPIOB->MODER &= ~(3 << (i * 2)); 

        GPIOB->MODER |= (1 << (i * 2)); 

        GPIOB->ODR |= (1 << i); 

    } 

    // Cols PB4–PB7 input with pull-up 

    for (int i = 4; i < 8; i++) { 

        GPIOB->MODER &= ~(3 << (i * 2)); 

        GPIOB->PUPDR &= ~(3 << (i * 2)); 

        GPIOB->PUPDR |= (1 << (i * 2)); 

    } 

} 

 

// === Keypad Scan === 

char Keypad_Scan(void) { 

    for (int row = 0; row < 4; row++) { 

        GPIOB->ODR |= 0x0F; 

        GPIOB->ODR &= ~(1 << row); 

        delayms(2); 

        for (int col = 0; col < 4; col++) { 

            if ((GPIOB->IDR & (1 << (col + 4))) == 0) { 

                delayms(20); 

                while ((GPIOB->IDR & (1 << (col + 4))) == 0); 

                return keymap[row][col]; 

            } 

        } 

    } 

    return 0; 

} 

 

// === Rest Functions (Motor, ADC, LED) are unchanged from second code === 

// ... 

 