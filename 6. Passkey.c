#include <string.h> 

#include <stdint.h> 

#include <stdio.h> 

#include <lcd.h> 

#include <cmn.h> 

#include "stm32f405xx.h" 

 

char passkey[5] = "1234"; 

char entered[5]; 

int key_index = 0; 

 

const char keymap[4][4] = 

{ 

    {'1', '2', '3', 'A'}, 

    {'4', '5', '6', 'B'}, 

    {'7', '8', '9', 'C'}, 

    {'*', '0', '#', 'D'} 

}; 

 

void GPIO_Init(void); 

char Keypad_Scan(void); 

 

int main(void) 

{ 

LcdInit(); 

    GPIO_Init(); 

 

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

                            GPIOC->ODR |= (1 << 10);     // Green LED ON 

                            GPIOC->ODR &= ~(1 << 11);    // Red LED OFF 

                            delayms(300); 

                            break; // Exit loop on success 

                        } 

                        else 

                        { 

                            lprint(0x80, "Wrong Passkey!   "); 

                            GPIOC->ODR |= (1 << 11);     // Red LED ON 

                            GPIOC->ODR &= ~(1 << 10);    // Green LED OFF 

                            delayms(200); 

                            key_index = 0; 

                            memset(entered, 0, sizeof(entered)); 

                            lprint(0x80, "Try Again!       "); 

                            delayms(1000); 

                            lprint(0x80, "Input Your Passkey"); 

                            lprint(0xC0, "                "); 

                        } 

                    } 

                    else if (key_index < 4) 

                    { 

                        entered[key_index++] = key; 

 

                        char buf[17] = "Entered: "; 

                        for (int i = 0; i < key_index; i++) 

                            buf[9 + i] = '*'; 

                        buf[9 + key_index] = '\0'; 

                        lprint(0xC0, buf); 

                    } 

                } 

            } while (1); // Repeat until correct passkey 

        } 

void GPIO_Init(void) 

{ 

    // Enable clocks for GPIOB, GPIOC, GPIOD (if needed) 

    RCC->AHB1ENR |= (1 << 0) |(1 << 1) | (1 << 2) | (1 << 3); 

 

    // PC10 and PC11 as output (LEDs) 

    GPIOC->MODER &= ~(0xF << 20);      // Clear MODER10 and MODER11 

    GPIOC->MODER |= (1 << 20) | (1 << 22); // Output mode 

 

    // Clear LEDs 

    GPIOC->ODR &= ~((1 << 10) | (1 << 11)); 

 

    // Configure PB0–PB3 (Rows) as output 

    for (int i = 0; i < 4; i++) 

    { 

        GPIOB->MODER &= ~(3 << (i * 2)); 

        GPIOB->MODER |= (1 << (i * 2));  // Output mode 

        GPIOB->ODR |= (1 << i);          // Set high initially 

    } 

 

    // Configure PB4–PB7 (Cols) as input with pull-up 

    for (int i = 4; i < 8; i++) 

    { 

        GPIOB->MODER &= ~(3 << (i * 2)); // Input mode 

        GPIOB->PUPDR &= ~(3 << (i * 2)); 

        GPIOB->PUPDR |= (1 << (i * 2));  // Pull-up 

    } 

//    for (int i =2 ; i <4 ; i++) 

//        { 

//            GPIOA->MODER &= ~(3 << (i * 2)); // Input mode 

//            GPIOA->PUPDR &= ~(3 << (i * 2)); 

//            GPIOA->PUPDR |= (1 << (i * 2));  // Pull-up 

//        } 

} 

 

char Keypad_Scan(void) 

{ 

    for (int row = 0; row < 4; row++) 

    { 

        // Set all rows HIGH 

        GPIOB->ODR |= 0x0F; 

 

        // Pull current row LOW 

        GPIOB->ODR &= ~(1 << row); 

        delayms(2);  // Debounce delay 

 

        // Scan columns PB4, PB5 (col 0, col 1) 

        for (int col = 0; col < 4; col++) 

        { 

            if ((GPIOB->IDR & (1 << (col + 4))) == 0) 

            { 

                delayms(20);  // Debounce 

                while ((GPIOB->IDR & (1 << (col + 4))) == 0);  // Wait for release 

                return keymap[row][col]; 

            } 

        } 

 

//        // Scan columns PA2, PA3 (col 2, col 3) 

//        for (int col = 0; col < 2; col++) 

//        { 

//            if ((GPIOA->IDR & (1 << (col + 2))) == 0) 

//            { 

//                delayms(20);  // Debounce 

//                while ((GPIOA->IDR & (1 << (col + 2))) == 0);  // Wait for release 

//                return keymap[row][col + 2]; 

//            } 

//        } 

    } 

    return 0; 

} 

 