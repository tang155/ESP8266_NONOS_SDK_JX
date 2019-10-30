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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//																//																//
// 工程：	AP_TCP_Client										//	①：ESP8266复位后，电脑接入ESP8266建立的WIFI				//
//																//																//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0				//	②：打开网络调试软件，新建TCP_Server						//
//																//																//
// 功能：	①：ESP8266设置为AP模式，建立WIFI热点				//	③：设置Server(电脑)：IP=192.168.4.2、端口=8888				//
//																//																//
//			②：等待30秒(等待电脑接入WIFI、开启TCP_Server)		//	④：启动TCP_Server，建立侦听(需要在ESP8266复位后30秒内完成)	//
//																//																//
//			③：创建TCP_Client(ESP8266)，连接TCP_Server(电脑)	//	⑤：等待TCP_Client(ESP8266)向TCP_Server(电脑)建立TCP连接	//
//																//																//
//			④：连接成功后，主动向TCP_Server发起通信(打招呼)	//	⑥：TCP_Server(电脑端)会收到TCP_Client(ESP8266)的问候消息	//
//																//																//
//			⑤：等待TCP_Server发送数据过来						//	⑦：在"客户端列表"中选择"192.168.4.1"(ESP8266)				//
//																//																//
//			⑥：根据接收到的数据控制LED的亮/灭					//	⑧：向ESP8266发送数据，控制局域网内灯的亮/灭				//
//																//																//
//	版本：	V1.0												//	⑨：【首字母为'k'/'K'，灯亮】、【首字母为'g'/'G'，灯灭】	//
//																//																//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
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
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"AP_TCP_Client"		// 工程名宏定义

#define		ESP8266_AP_SSID		"ESP8266_JX"		// 创建的WIFI名
#define		ESP8266_AP_PASS		"jixiaoxin"			// 创建的WIFI密码

#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================


// 全局变量
//==================================================================================
os_timer_t OS_Timer_1;			// 定义软件定时器

struct espconn ST_NetCon;		// 网络连接结构体
//==================================================================================

// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================

// 初始化ESP8266_AP模式
//============================================================================================
void ICACHE_FLASH_ATTR ESP8266_AP_Init_JX()
{
	struct softap_config AP_Config;				// AP参数结构体

	wifi_set_opmode(0x02);						// 设置为AP模式，并保存到Flash

	// 结构体赋值(注意：【服务集标识符/密码】须设为字符串形式)
	//--------------------------------------------------------------------------------------
	os_memset(&AP_Config, 0, sizeof(struct softap_config));	// AP参数结构体 = 0
	os_strcpy(AP_Config.ssid,ESP8266_AP_SSID);		// 设置SSID(将字符串复制到ssid数组)
	os_strcpy(AP_Config.password,ESP8266_AP_PASS);	// 设置密码(将字符串复制到password数组)
	AP_Config.ssid_len=os_strlen(ESP8266_AP_SSID);	// 设置ssid长度(和SSID的长度一致)
	AP_Config.channel=1;                      		// 通道号1～13
	AP_Config.authmode=AUTH_WPA2_PSK;           	// 设置加密模式
	AP_Config.ssid_hidden=0;                  		// 不隐藏SSID
	AP_Config.max_connection=4;               		// 最大连接数
	AP_Config.beacon_interval=100;            		// 信标间隔时槽100～60000 ms

	wifi_softap_set_config(&AP_Config);				// 设置soft-AP，并保存到Flash
}
//============================================================================================


// 成功发送网络数据的回调函数
//==========================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send_Cb_JX(void *arg)
{
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//==========================================================

// 成功接收网络数据的回调函数【参数1：网络传输结构体espconn指针、参数2：网络传输数据指针、参数3：数据长度】
//=========================================================================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv_Cb_JX(void * arg, char * pdata, unsigned short len)
{
	struct espconn * T_arg = arg;		// 缓存网络连接结构体指针

	// 根据数据设置LED的亮/灭
	//-------------------------------------------------------------------------------
	if(pdata[0] == 'k' || pdata[0] == 'K')	LED_ON;			// 首字母为'k'/'K'，灯亮
	else if(pdata[0] == 'g' || pdata[0] == 'G')	LED_OFF;	// 首字母为'g'/'G'，灯灭
	os_printf("\nESP8266_Receive_Data = %s\n",pdata);		// 串口打印接收到的数据


	/*
	// 获取远端信息
	//------------------------------------------------------------------------------------
	remot_info * P_port_info = NULL;	// 定义远端连接信息指针
	if(espconn_get_connection_info(T_arg, &P_port_info, 0)==ESPCONN_OK)	// 获取远端信息
	{
		T_arg->proto.tcp->remote_port  = P_port_info->remote_port;	// 获取对方端口号
		T_arg->proto.tcp->remote_ip[0] = P_port_info->remote_ip[0];	// 获取对方的IP地址
		T_arg->proto.tcp->remote_ip[1] = P_port_info->remote_ip[1];
		T_arg->proto.tcp->remote_ip[2] = P_port_info->remote_ip[2];
		T_arg->proto.tcp->remote_ip[3] = P_port_info->remote_ip[3];
		//os_memcpy(T_arg->proto.tcp->remote_ip,P_port_info->remote_ip,4);	// 内存拷贝
	}
	*/

	//--------------------------------------------------------------------
	OLED_ShowIP(24,6,T_arg->proto.tcp->remote_ip);	// 显示远端主机IP地址
	//--------------------------------------------------------------------

	//【TCP通信是面向连接的，向远端主机回应时可直接使用T_arg结构体指针指向的IP信息】
	//-----------------------------------------------------------------------------------------------
	espconn_send(T_arg,"ESP8266_WIFI_Recv_OK",os_strlen("ESP8266_WIFI_Recv_OK"));	// 向对方发送应答
}
//=========================================================================================================


// TCP连接断开成功的回调函数
//================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Disconnect_Cb_JX(void *arg)
{
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
}
//================================================================


// TCP连接建立成功的回调函数
//====================================================================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Connect_Cb_JX(void *arg)
{
	espconn_regist_sentcb((struct espconn *)arg, ESP8266_WIFI_Send_Cb_JX);			// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb((struct espconn *)arg, ESP8266_WIFI_Recv_Cb_JX);			// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb((struct espconn *)arg,ESP8266_TCP_Disconnect_Cb_JX);	// 注册成功断开TCP连接的回调函数

	espconn_send((struct espconn *)arg,"Hello,I am ESP8266",os_strlen("Hello,I am ESP8266"));	// 向Server发起通信

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
}
//====================================================================================================================


// TCP连接异常断开时的回调函数
//====================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Break_Cb_JX(void *arg,sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");

	espconn_connect(&ST_NetCon);	// 连接TCP-server
}
//====================================================================


// 定义espconn型结构体
//-----------------------------------------------
//struct espconn ST_NetCon;	// 网络连接结构体

// 初始化网络连接(TCP通信)
//========================================================================================================
void ICACHE_FLASH_ATTR ESP8266_NetCon_Init_JX()
{
	// 结构体赋值
	//--------------------------------------------------------------------------
	ST_NetCon.type = ESPCONN_TCP ;				// 设置为TCP协议

	ST_NetCon.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));	// 开辟内存


	// 此处需要设置目标IP/端口(ESP8266作为Client，需要预先知道Server的IP/端口)
	//-------------------------------------------------------------------------
	ST_NetCon.proto.tcp->local_port = 8266 ;	// 设置本地端口
	ST_NetCon.proto.tcp->remote_port = 8888;	// 设置目标端口
	ST_NetCon.proto.tcp->remote_ip[0] = 192;	// 设置目标IP地址
	ST_NetCon.proto.tcp->remote_ip[1] = 168;
	ST_NetCon.proto.tcp->remote_ip[2] = 4;
	ST_NetCon.proto.tcp->remote_ip[3] = 2;
	//u8 remote_ip[4] = {192,168,4,2};		// 目标ip地址
	//os_memcpy(ST_NetCon.proto.udp->remote_ip,remote_ip,4);	// 拷贝内存


	// 注册连接成功回调函数、异常断开回调函数
	//--------------------------------------------------------------------------------------------------
	espconn_regist_connectcb(&ST_NetCon, ESP8266_TCP_Connect_Cb_JX);	// 注册TCP连接成功建立的回调函数
	espconn_regist_reconcb(&ST_NetCon, ESP8266_TCP_Break_Cb_JX);		// 注册TCP连接异常断开的回调函数


	// 连接 TCP server
	//----------------------------------------------------------
	espconn_connect(&ST_NetCon);	// 连接TCP-server
}
//========================================================================================================


// 软件定时的回调函数
//==========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	struct ip_info ST_ESP8266_IP;	// IP信息结构体
	u8 ESP8266_IP[4];		// 点分十进制形式保存IP


	wifi_get_ip_info(SOFTAP_IF,&ST_ESP8266_IP);	// 查询AP模式下ESP8266的IP地址

	if(ST_ESP8266_IP.ip.addr!=0 )				// ESP8266成功获取到IP地址
	{
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// 点分十进制IP的第一个数 <==> addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// 点分十进制IP的第二个数 <==> addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// 点分十进制IP的第三个数 <==> addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// 点分十进制IP的第四个数 <==> addr高八位

		// 显示ESP8266的IP地址
		//-----------------------------------------------------------------------------------------------
		os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
		//-----------------------------------------------------------------------------------------------


		os_timer_disarm(&OS_Timer_1);	// 关闭定时器

		ESP8266_NetCon_Init_JX();		// 初始化网络连接(TCP通信)
	}
}
//==========================================================================================================


// 软件定时器初始化(ms毫秒)
//=====================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init_JX(u32 time_ms, u8 time_repetitive)
{

	os_timer_disarm(&OS_Timer_1);	// 关闭定时器
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // 使能定时器
}
//=====================================================================================


// LED初始化
//=============================================================================
void ICACHE_FLASH_ATTR LED_Init_JX(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4设为IO口

	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// IO4 = 1
}
//=============================================================================


// user_init：entry of user application, init user function here
//=============================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(115200,115200);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");

	// OLED显示初始化
	//------------------------------------------------------------
	OLED_Init();								// OLED初始化
	OLED_ShowString(0,0,"ESP8266 = Client");	// ESP8266模式
	OLED_ShowString(0,2,"IP:");					// ESP8266_IP地址
	OLED_ShowString(0,4,"Remote  = Server");	// 远端主机模式
	OLED_ShowString(0,6,"IP:");					// 远端主机IP地址
	//------------------------------------------------------------

	LED_Init_JX();		// LED初始化


	ESP8266_AP_Init_JX();			// 初始化ESP8266_AP模式

	OS_Timer_1_Init_JX(30000,0);	// 30秒定时(一次)
}
//=============================================================================


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
