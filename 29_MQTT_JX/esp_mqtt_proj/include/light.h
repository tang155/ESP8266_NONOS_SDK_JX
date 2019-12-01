
#ifndef APP_INCLUDE_DRIVER_LED_H_
#define APP_INCLUDE_DRIVER_LED_H_

#include "c_types.h"


extern u8 LED_State;

void LED_Init(void);//ÉùÃ÷º¯Êý

void GLED_OFF(void);
void RLED_OFF(void);
void BLED_OFF(void);

void GLED_ON(void);
void RLED_ON(void);
void BLED_ON(void);

void LED_Flash(void);

#endif /* APP_INCLUDE_DRIVER_LED_H_ */
