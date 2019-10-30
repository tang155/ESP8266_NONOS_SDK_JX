/* * ESPRESSIF MIT License
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//															//												//
// 工程：	SNTP											//		①：修改WIFI名、WIFI密码，接入WIFI		//
//															//												//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0			//		②：复位8266，等待8266接入WIFI			//
//															//												//
// 功能：	①：8266设置为STA模式，接入路由器WIFI			//		③：对SNTP_API所得时间格式做了修改		//
//															//												//
//			②：设置SNTP服务器，初始化SNTP					//		④：OLED_1行：【年、月、日、星期X】		//
//															//												//
//			③：每秒查询Internet时间						//		⑤：OLED_2行：【Clock = 时、分、秒】	//
//															//												//
//			④：每5秒读取DHT11检测的温湿度					//		⑥：OLED_3行：【Hunid = 湿度 %RH】		//
//															//												//
//			⑤：OLED显示【Internet时间】、【环境温湿度】"	//		⑦：OLED_4行：【Temp  = 温度 ℃】		//
//															//												//
//	版本：	V1.0											//		⑧：8266和OLED通信时，蓝色OLED闪烁		//
//															//												//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。
#include "espconn.h"			// TCP/UDP接口
//#include "espnow.h"
#include "ets_sys.h"			// 回调函数
//#include "gpio.h"
#include "mem.h"				// 内存申请等函数
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
//#include "ping.h"
//#include "pwm.h"
//#include "queue.h"
//#include "smartconfig.h"
#include "sntp.h"				// SNTP
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"SNTP"					// 工程名宏定义

#define		ESP8266_STA_SSID	"5G_TEST"			// 接入的WIFI名
#define		ESP8266_STA_PASS	"15980847344"		// 接入的WIFI密码

#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================

// 全局变量
//==================================================================================
u8 C_Read_DHT11 = 0;			// 读取DHT11计时

os_timer_t OS_Timer_IP;			// 定时器_查询IP地址

os_timer_t OS_Timer_SNTP;		// 定时器_SNTP
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// ESP8266_STA初始化
//=========================================================================================
void ICACHE_FLASH_ATTR ESP8266_STA_Init_JX()
{
	struct station_config STA_Config;	// STA参数结构体

	struct ip_info ST_ESP8266_IP;		// STA信息结构体

	// 设置ESP8266的工作模式
	//------------------------------------------------------------------------
	wifi_set_opmode(0x01);				// 设置为STA模式，并保存到Flash

	/*
	// 设置STA模式下的IP地址【ESP8266默认开启DHCP Client，接入WIFI时会自动分配IP地址】
	//--------------------------------------------------------------------------------
	wifi_station_dhcpc_stop();						// 关闭 DHCP Client
	IP4_ADDR(&ST_ESP8266_IP.ip,192,168,8,35);		// 配置IP地址
	IP4_ADDR(&ST_ESP8266_IP.gw,192,168,8,1);		// 配置网关地址
	IP4_ADDR(&ST_ESP8266_IP.netmask,255,255,255,0);	// 配置子网掩码
	wifi_set_ip_info(STATION_IF,&ST_ESP8266_IP);	// 设置STA模式下的IP地址
	*/

	// 结构体赋值，配置STA模式参数
	//-------------------------------------------------------------------------------
	os_memset(&STA_Config, 0, sizeof(struct station_config));	// STA参数结构体 = 0
	os_strcpy(STA_Config.ssid,ESP8266_STA_SSID);				// 设置WIFI名
	os_strcpy(STA_Config.password,ESP8266_STA_PASS);			// 设置WIFI密码

	wifi_station_set_config(&STA_Config);	// 设置STA参数，并保存到Flash

	// wifi_station_connect();		// ESP8266连接WIFI
}
//=========================================================================================



//	 -----------------------------------------------			 -----------------------------------------------
//	|	  月份		|	英文简写	|	英文全称	|			|	  星期X		|	英文简写	|	英文全称	|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  一月		|	  Jan		|	January		|			|	  周一		|	  Mon		|	Monday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|	  二月		|	  Feb		|	February	|			|	  周二		|	  Tue		|	Tuesday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  三月		|	  Mar		|	March		|			|	  周三		|	  Wed		|	Wednesday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  四月		|	  Apr		|	April		|			|	  周四		|	  Thu		|	Thursday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  五月		|	  May		|	May			|			|	  周五		|	  Fri		|	Friday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  六月		|	  June		|	June		|			|	  周六		|	  Sat		|	Saturday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  七月		|	  July		|	July		|			|	  周日		|	  Sun		|	Sunday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  八月		|	  Aug		|	Aguest		|
//	 -----------------------------------------------
//	|  	  九月		|	  Sept		|	September	|
//	 -----------------------------------------------
//	|  	  十月		|	  Oct		|	October		|
//	 -----------------------------------------------
//	|  	 十一月		|	  Nov		|	November	|
//	 -----------------------------------------------
//	|  	 十二月		|	  Dec		|	December	|
//	 -----------------------------------------------


// SNTP定时回调函数
//===================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_SNTP_cb(void	 * arg)
{
	// 字符串整理 相关变量
	//------------------------------------------------------

	u8 C_Str = 0;				// 字符串字节计数

	char A_Str_Data[20] = {0};	// 【"日期"】字符串数组

	char *T_A_Str_Data = A_Str_Data;	// 缓存数组指针

	char A_Str_Clock[10] = {0};	// 【"时间"】字符串数组


	char * Str_Head_Week;		// 【"星期"】字符串首地址

	char * Str_Head_Month;		// 【"月份"】字符串首地址

	char * Str_Head_Day;		// 【"日数"】字符串首地址

	char * Str_Head_Clock;		// 【"时钟"】字符串首地址

	char * Str_Head_Year;		// 【"年份"】字符串首地址

	//------------------------------------------------------


	 uint32	TimeStamp;		// 时间戳

	 char * Str_RealTime;	// 实际时间的字符串


	 // 查询当前距离基准时间(1970.01.01 00:00:00 GMT+8)的时间戳(单位:秒)
	 //-----------------------------------------------------------------
	 TimeStamp = sntp_get_current_timestamp();

	 if(TimeStamp)		// 判断是否获取到偏移时间
	 {
		 //os_timer_disarm(&OS_Timer_SNTP);	// 关闭SNTP定时器

		 // 查询实际时间(GMT+8):东八区(北京时间)
		 //--------------------------------------------
		 Str_RealTime = sntp_get_real_time(TimeStamp);


		 // 【实际时间】字符串 == "周 月 日 时:分:秒 年"
		 //------------------------------------------------------------------------
		 os_printf("\r\n----------------------------------------------------\r\n");
		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// 时间戳
		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// 实际时间
		 os_printf("--------------------------------------------------------\r\n");


		 // 时间字符串整理，OLED显示【"日期"】、【"时间"】字符串
		 //…………………………………………………………………………………………………………………

		 // 【"年份" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 Str_Head_Year = Str_RealTime;	// 设置起始地址

		 while( *Str_Head_Year )		// 找到【"实际时间"】字符串的结束字符'\0'
			 Str_Head_Year ++ ;

		 // 【注：API返回的实际时间字符串，最后还有一个换行符，所以这里 -5】
		 //-----------------------------------------------------------------
		 Str_Head_Year -= 5 ;			// 获取【"年份"】字符串的首地址

		 T_A_Str_Data[4] = ' ' ;
		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// 【"年份" + ' '】填入日期数组

		 T_A_Str_Data += 5;				// 指向【"年份" + ' '】字符串的后面的地址
		 //---------------------------------------------------------------------------------

		 // 获取【日期】字符串的首地址
		 //---------------------------------------------------------------------------------
		 Str_Head_Week 	= Str_RealTime;							// "星期" 字符串的首地址
		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "月份" 字符串的首地址
		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "日数" 字符串的首地址
		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "时钟" 字符串的首地址


		 // 【"月份" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Day - Str_Head_Month;				// 【"月份" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// 【"月份" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"月份" + ' '】字符串的后面的地址


		 // 【"日数" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Clock - Str_Head_Day;				// 【"日数" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// 【"日数" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"日数" + ' '】字符串的后面的地址


		 // 【"星期" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// 【"星期"】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// 【"星期"】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"星期"】字符串的后面的地址


		 // OLED显示【"日期"】、【"时钟"】字符串
		 //---------------------------------------------------------------------------------
		 *T_A_Str_Data = '\0';		// 【"日期"】字符串后面添加'\0'

		 OLED_ShowString(0,0,A_Str_Data);		// OLED显示日期


		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// 【"时钟"】字符串填入时钟数组
		 A_Str_Clock[8] = '\0';

		 OLED_ShowString(64,2,A_Str_Clock);		// OLED显示时间

		 //…………………………………………………………………………………………………………………
	 }

	// 每5秒，读取/显示温湿度数据
	//-----------------------------------------------------------------------------------------
	C_Read_DHT11 ++ ;		// 读取DHT11计时

	if(C_Read_DHT11>=5)		// 5秒计时
	{
		C_Read_DHT11 = 0;	// 计时=0

		if(DHT11_Read_Data_Complete() == 0)		// 读取DHT11温湿度
		{
			DHT11_NUM_Char();	// DHT11数据值转成字符串

			OLED_ShowString(64,4,DHT11_Data_Char[1]);	// DHT11_Data_Char[0] == 【温度字符串】
			OLED_ShowString(64,6,DHT11_Data_Char[0]);	// DHT11_Data_Char[1] == 【湿度字符串】
		}

		else
		{
			OLED_ShowString(64,4,"----");	// Temperature：温度
			OLED_ShowString(64,6,"----");	// Humidity：湿度
		}
	}
	//-----------------------------------------------------------------------------------------
}
//===================================================================================================


// SNTP定时初始化
//=============================================================================
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Init_JX(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_SNTP);
	os_timer_setfn(&OS_Timer_SNTP,(os_timer_func_t *)OS_Timer_SNTP_cb,NULL);
	os_timer_arm(&OS_Timer_SNTP, time_ms, time_repetitive);
}
//=============================================================================


// 初始化SNTP
//=============================================================================
void ICACHE_FLASH_ATTR ESP8266_SNTP_Init_JX(void)
{
	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

	sntp_setservername(0, "us.pool.ntp.org");	// 服务器_0【域名】
	sntp_setservername(1, "ntp.sjtu.edu.cn");	// 服务器_1【域名】

	ipaddr_aton("210.72.145.44", addr);			// 点分十进制 => 32位二进制
	sntp_setserver(2, addr);					// 服务器_2【IP地址】
	os_free(addr);								// 释放addr

	sntp_init();	// SNTP初始化API

	OS_Timer_SNTP_Init_JX(1000,1);				// 1秒重复定时(SNTP)
}
//=============================================================================


// IP定时的回调函数
//=========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	u8 C_LED_Flash = 0;				// LED闪烁计次

	struct ip_info ST_ESP8266_IP;	// ESP8266的IP信息
	u8 ESP8266_IP[4];				// ESP8266的IP地址


	// 成功接入WIFI【STA模式下，如果开启DHCP(默认)，则ESO8266的IP地址由WIFI路由器自动分配】
	//-------------------------------------------------------------------------------------
	if( wifi_station_get_connect_status() == STATION_GOT_IP )	// 判断是否获取IP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取STA的IP信息
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// IP地址低八位 == addr高八位

		// 显示ESP8266的IP地址
		//------------------------------------------------------------------------------------------------
		//os_printf("\nESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		//OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
		//------------------------------------------------------------------------------------------------

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		ESP8266_SNTP_Init_JX();			// 初始化SNTP
	}
}
//=========================================================================================================

// 软件定时器初始化(ms毫秒)
//========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_Init_JX(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_IP);	// 关闭定时器
	os_timer_setfn(&OS_Timer_IP,(os_timer_func_t *)OS_Timer_IP_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_IP, time_ms, time_repetitive);  // 使能定时器
}
//========================================================================================

// LED初始化
//=============================================================================
void ICACHE_FLASH_ATTR LED_Init_JX(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4设为IO口

	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// IO4 = 1
}
//=============================================================================


// user_init：entry of user application, init user function here
//==============================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(115200,115200);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");

	// OLED显示初始化
	//----------------------------------------------------------------
	OLED_Init();								// OLED初始化
	OLED_ShowString(0,0,"                ");	// Internet Time
	OLED_ShowString(0,2,"Clock =         ");	// Clock：时钟
	OLED_ShowString(0,4,"Temp  =         ");	// Temperature：温度
	OLED_ShowString(0,6,"Humid =         ");	// Humidity：湿度
	//----------------------------------------------------------------

	LED_Init_JX();		// LED初始化


	ESP8266_STA_Init_JX();			// ESP8266_STA初始化

	OS_Timer_IP_Init_JX(1000,1);	// 1秒重复定时(获取IP地址)
}
//==============================================================================


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
