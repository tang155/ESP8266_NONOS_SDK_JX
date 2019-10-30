/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//////////////////////////////////////////////////////////
//														//
// 工程：	OS_Timer									//
//														//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0		//
//														//
// 功能：	①：初始化LED(GPIO4)						//
//														//
//			②：初始化软件定时器(500ms)					//
//														//
//			③：LED每500Ms翻转一次						//
//														//
// 版本：	V1.0										//
//														//
//////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
//#include "espconn.h"
//#include "espnow.h"
#include "ets_sys.h"			// 回调函数
//#include "gpio.h"
//#include "ip_addr.h"
//#include "mem.h"
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
//#include "ping.h"
//#include "pwm.h"
//#include "queue.h"
//#include "smartconfig.h"
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================

// 宏定义
//==================================================================================
#define		ProjectName		"OS_Timer"		// 工程名宏定义
//==================================================================================


// 全局变量
//==================================================================================
u8 F_LED = 0;				// LED状态标志位

// os_timer_t OS_Timer_1;	// 软件定时器
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// 注：OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
//--------------------------------------------------------------------
os_timer_t OS_Timer_1;	// ①：定义软件定时器(os_timer_t型结构体)

// 软件定时的回调函数
//======================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)		// ②：定义回调函数
{
	F_LED = !F_LED;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),F_LED);		// LED状态翻转


	os_printf("\r\n----OS_Timer_1_cb----\r\n");	// 进入回调函数标志
}
//======================================================================


// 软件定时器初始化(ms毫秒)
//================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init_JX(u32 time_ms, u8 time_repetitive)
{
	// 关闭定时器
	// 参数一：要关闭的定时器
	//--------------------------------------------------------
	os_timer_disarm(&OS_Timer_1);	// ③：关闭软件定时器


	// 设置定时器
	// 参数一：要设置的定时器；参数二：回调函数(需类型转换)；参数三：回调函数的参数
	//【注：os_timer_setfn必须在软件定时器未使能的情况下调用】
	//------------------------------------------------------------------------------------------
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);	// ④：设置回调函数


	// 使能(启动)ms定时器
	// 参数一：要使能的定时器；参数二：定时时间（单位：ms）；参数三：1=重复/0=只一次
	//------------------------------------------------------------------------------------------
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // ⑤：设置定时器参数并使能定时器
	//-------------------------------------------------------------------
	// 【如未调用system_timer_reinit，可支持范围：[5ms ～ 6,870,947ms]】
	// 【如果调用system_timer_reinit，可支持范围：[100ms ～ 428,496 ms]】
	//-------------------------------------------------------------------
}
//================================================================================================

// LED初始化
//=============================================================================
void ICACHE_FLASH_ATTR LED_Init_JX(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4设为IO口

	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// IO4 = 1
}
//=============================================================================

// user_init：entry of user application, init user function here
//=================================================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(115200,115200);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");


	LED_Init_JX();					// LED初始化


	// 初始化软件定时器
	//--------------------------------------------
	OS_Timer_1_Init_JX(500,1);		// 500ms(重复)



//	while(1) system_soft_wdt_feed();	// 死循环，测试用

	os_printf("\r\n-------------------- user_init OVER --------------------\r\n");
}
//=================================================================================================


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void){}
