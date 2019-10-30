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
// 工程：	DHT11										//
//														//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0		//
//														//
// 功能：	①：初始化LED、软件定时器					//
//														//
//			②：3秒重复定时								//
//														//
//			③：每3秒读取一次DHT11温湿度				//
//														//
//			④：温度超过30℃，LED亮						//
//														//
//			⑤：串口、OLED显示温湿度					//
//														//
// 版本：	V1.0										//
//														//
//////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11头文件

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
//#include "espconn.h"
//#include "espnow.h"
#include "ets_sys.h"			// 回调函数
//#include "gpio.h"				//
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
#define		ProjectName			"DHT11"			// 工程名宏定义
//==================================================================================


// 全局变量
//==================================================================================
u8 F_LED = 0;				// LED状态标志位

os_timer_t OS_Timer_1;		// 软件定时器
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// 定时的回调函数
//==========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	if(DHT11_Read_Data_Complete() == 0)		// 读取DHT11温湿度值
	{
		//-------------------------------------------------
		// DHT11_Data_Array[0] == 湿度_整数_部分
		// DHT11_Data_Array[1] == 湿度_小数_部分
		// DHT11_Data_Array[2] == 温度_整数_部分
		// DHT11_Data_Array[3] == 温度_小数_部分
		// DHT11_Data_Array[4] == 校验字节
		// DHT11_Data_Array[5] == 【1:温度>=0】【0:温度<0】
		//-------------------------------------------------


		// 温度超过30℃，LED亮
		//----------------------------------------------------
		if(DHT11_Data_Array[5]==1 && DHT11_Data_Array[2]>=30)
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED亮
		else
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// LED灭


		// 串口输出温湿度
		//---------------------------------------------------------------------------------
		if(DHT11_Data_Array[5] == 1)			// 温度 >= 0℃
		{
			os_printf("\r\n湿度 == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
			os_printf("\r\n温度 == %d.%d ℃\r\n", DHT11_Data_Array[2],DHT11_Data_Array[3]);
		}
		else // if(DHT11_Data_Array[5] == 0)	// 温度 < 0℃
		{
			os_printf("\r\n湿度 == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
			os_printf("\r\n温度 == -%d.%d ℃\r\n",DHT11_Data_Array[2],DHT11_Data_Array[3]);
		}

		// OLED显示温湿度
		//---------------------------------------------------------------------------------
		DHT11_NUM_Char();	// DHT11数据值转成字符串

		OLED_ShowString(0,2,DHT11_Data_Char[0]);	// DHT11_Data_Char[0] == 【湿度字符串】
		OLED_ShowString(0,6,DHT11_Data_Char[1]);	// DHT11_Data_Char[1] == 【温度字符串】
	}
}
//==========================================================================================

// 软件定时器初始化(ms毫秒)
//==========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init_JX(u32 time_ms, u8 time_repetitive)
{

	os_timer_disarm(&OS_Timer_1);	// 关闭定时器
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // 使能定时器
}
//==========================================================================================

// LED初始化
//==========================================================================================
void ICACHE_FLASH_ATTR LED_Init_JX(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4设为IO口

	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// IO4 = 1
}
//==========================================================================================

// user_init：entry of user application, init user function here
//==========================================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(115200,115200);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");

	// OLED显示初始化
	//--------------------------------------------------------
	OLED_Init();							// OLED初始化
	OLED_ShowString(0,0,"Humidity:");		// 湿度
	OLED_ShowString(0,4,"Temperature:");	// 温度
	//--------------------------------------------------------

	LED_Init_JX();		// LED初始化

	OS_Timer_1_Init_JX(3000,1);		// 3秒定时(重复)
}
//==========================================================================================


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
