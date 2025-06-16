#include "stm32f405xx.h" 

#include "lcd.h" 

#include "cmn.h" 

#include <stdio.h> 

 

#include <stdint.h> 

 

uint16_t result=0; 

 

// ADC Initialization 

 

void adc_init(void) 

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

uint16_t adc_read(void) 

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

 

// Turn off all LEDs 

 

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

 

int main() 

 

{ 

 

    //uint16_t result; 

 

    adc_init();  // Initialize ADC for potentiometer reading 

    char buffer[17]; 

    led_init();  // Initialize GPIO for LEDs 

    LcdInit(); 

 

    while (1) 

 

    { 

 

        result = adc_read(); // Read ADC value from potentiometer 

        int gas_percent = (result * 100) / 4095; 

        if(gas_percent>30){ 

        sprintf(buffer, "Gas: %3d%%       ", gas_percent); 

        lprint(0xc0, buffer); 

        lprint(0x80, "Gas detected"); 

        } 

        else 

        { 

        	LcdFxn(0,0x01); 

        } 

        led_control(result); // Control LED based on ADC value 

 

        for (uint32_t j = 0; j < 20000; j++) {} // Small delay to make the change visible 

 

    } 

 

    return 0; 

 

} 

 