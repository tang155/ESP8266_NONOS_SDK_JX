
#include <ets_sys.h>
#include <gpio.h>
#include "light.h"

u8 LED_State=0;

void ICACHE_FLASH_ATTR LED_Init(void)
{
	//配置LED连接管脚为GPIO功能
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U , FUNC_GPIO13);
}

void ICACHE_FLASH_ATTR RLED_ON(void)
{
	LED_State=1;
	GPIO_OUTPUT_SET(15, LED_State);//GPIO15输出高电平
}

void ICACHE_FLASH_ATTR GLED_ON(void)
{
	LED_State=1;
	GPIO_OUTPUT_SET(12, LED_State);//GPIO12输出高电平
}
void ICACHE_FLASH_ATTR BLED_ON(void)
{
	LED_State=1;
	GPIO_OUTPUT_SET(13, LED_State);//GPIO13输出高电平
}

void ICACHE_FLASH_ATTR RLED_OFF(void)
{
	LED_State=0;
	GPIO_OUTPUT_SET(15, LED_State);//GPIO15输出低电平
}
void ICACHE_FLASH_ATTR GLED_OFF(void)
{
	LED_State=0;
	GPIO_OUTPUT_SET(12, LED_State);//GPIO12输出低电平
}
void ICACHE_FLASH_ATTR BLED_OFF(void)
{
	LED_State=0;
	GPIO_OUTPUT_SET(13, LED_State);//GPIO13输出低电平
}

void ICACHE_FLASH_ATTR LED_Flash(void)
{
	LED_State=!LED_State;
	GPIO_OUTPUT_SET(12, LED_State);//LED状态取反
}
