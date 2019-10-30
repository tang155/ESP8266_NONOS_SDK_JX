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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//																//													//
// 工程：	DNS													//	①：打开命令提示符								//
//																//													//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0				//	②：输入【ping "www.rationmcu.com"】，回车		//
//																//													//
// 功能：	①：8266设置为STA模式，接入路由器WIFI				//	③：可以查询到"www.rationmcu.com"的IP地址		//
//																//													//
//			②：设置TCP通信参数									//	④：对照【串口/OLED】显示的IP地址，看看是否一致	//
//																//													//
//			③：解析域名"www.rationmcu.com"，获取服务器IP地址	//	⑤：如果两者一致，则说明ESP8266域名解析成功		//
//																//													//
//			④：串口/OLED显示"www.rationmcu.com"的IP地址		//	⑥：查看串口是否打印"TCP_Connect_OK"字符串		//
//																//													//
//			⑤：ESP8266作为TCP_Client，接入"www.rationmcu.com"	//	⑦：如果打印，说明ESP8266与服务器TCP连接成功	//
//																//													//
//			⑥：TCP连接成功后，串口显示"TCP_Connect_OK"字符串	//													//
//																//													//
//	版本：	V1.0												//													//
//																//													//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



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
#include "sntp.h"				// SNTP
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"DNS"					// 工程名宏定义

#define		ESP8266_STA_SSID	"5G_TEST"			// 接入的WIFI名
#define		ESP8266_STA_PASS	"15980847344"		// 接入的WIFI密码

#define 	DN_Server			"www.axentbath.com"		// 【瑞生网】域名


#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================

// 全局变量
//==================================================================================
os_timer_t OS_Timer_IP;			// 定时器_查询IP地址

struct espconn ST_NetCon;		// 网络连接结构体

ip_addr_t IP_Server;			// IP地址型结构体【32位二进制IP地址_服务器】
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
	os_printf("\nESP8266_Receive_Data = %s\n",pdata);		// 串口打印接收到的数据
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


// DNS_域名解析结束_回调函数【参数1：域名字符串指针 / 参数2：IP地址结构体指针 / 参数3：网络连接结构体指针】
//=========================================================================================================
void ICACHE_FLASH_ATTR DNS_Over_Cb_JX(const char * name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn * T_arg = (struct espconn *)arg;	// 缓存网络连接结构体指针

	//………………………………………………………………………………
	if(ipaddr == NULL)		// 域名解析失败
	{
		os_printf("\r\n---- DomainName Analyse Failed ----\r\n");

		return;
	}

	//……………………………………………………………………………………………………………
	else if (ipaddr != NULL && ipaddr->addr != 0)		// 域名解析成功
	{
		os_printf("\r\n---- DomainName Analyse Succeed ----\r\n");

		IP_Server.addr = ipaddr->addr;					// 获取服务器IP地址


		// 将解析到的服务器IP地址设为TCP连接的远端IP地址
		//------------------------------------------------------------------------------
		os_memcpy(T_arg->proto.tcp->remote_ip, &IP_Server.addr, 4);	// 设置服务器IP地址


		// 显示【瑞生网】的IP地址
		//------------------------------------------------------------------------------
		os_printf("\r\nIP_Server = %d.%d.%d.%d\r\n",						// 串口打印
				*((u8*)&IP_Server.addr),	*((u8*)&IP_Server.addr+1),
				*((u8*)&IP_Server.addr+2),	*((u8*)&IP_Server.addr+3));
		OLED_ShowIP(24,6,T_arg->proto.tcp->remote_ip);						// OLED显示
		//------------------------------------------------------------------------------

		// 注册连接成功回调函数、异常断开回调函数
		//----------------------------------------------------------------------------------------------
		//espconn_regist_connectcb(T_arg, ESP8266_TCP_Connect_Cb_JX);	// 注册TCP连接成功建立的回调函数
		//espconn_regist_reconcb(T_arg, ESP8266_TCP_Break_Cb_JX);		// 注册TCP连接异常断开的回调函数


		// 连接 TCP server
		//----------------------------------------------------------
		espconn_connect(T_arg);	// 连接TCP-server
	}
}
//=========================================================================================================



// 定义espconn型结构体
//-----------------------------------------------
//struct espconn ST_NetCon;	// 网络连接结构体

// 初始化网络连接(TCP通信)
//=========================================================================================================
void ICACHE_FLASH_ATTR ESP8266_NetCon_Init_JX()
{
	// 结构体赋值
	//--------------------------------------------------------------------------
	ST_NetCon.type = ESPCONN_TCP ;				// 设置为TCP协议

	ST_NetCon.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));	// 开辟内存


	// ESP8266作为TCP_Client，想要连接TCP_Server，必须知道TCP_Server的IP地址
	//--------------------------------------------------------------------------------
	ST_NetCon.proto.tcp->local_port = espconn_port() ;	// 本地端口【获取可用端口】
	ST_NetCon.proto.tcp->remote_port = 80;				// 目标端口【HTTP端口号80】
	//u8 remote_ip[4] = {121,198,34,91};				// 目标IP【www.rationmcu.com】
	//os_memcpy(ST_NetCon.proto.udp->remote_ip,remote_ip,4);

	// 获取域名所对应的IP地址
	//【参数1：网络连接结构体指针 / 参数2：域名字符串指针 / 参数3：IP地址结构体指针 / 参数4：回调函数】
	//-------------------------------------------------------------------------------------------------
	espconn_gethostbyname(&ST_NetCon, DN_Server, &IP_Server, DNS_Over_Cb_JX);


	// 注册连接成功回调函数、异常断开回调函数
	//--------------------------------------------------------------------------------------------------
	espconn_regist_connectcb(&ST_NetCon, ESP8266_TCP_Connect_Cb_JX);	// 注册TCP连接成功建立的回调函数
	espconn_regist_reconcb(&ST_NetCon, ESP8266_TCP_Break_Cb_JX);		// 注册TCP连接异常断开的回调函数


	// 连接 TCP server
	//----------------------------------------------------------
	//espconn_connect(&ST_NetCon);	// 连接TCP-server
}
//=========================================================================================================


// 软件定时的回调函数
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
		os_printf("\nESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
		//------------------------------------------------------------------------------------------------

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}


		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		ESP8266_NetCon_Init_JX();		// 初始化网络连接(TCP通信)
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
	//------------------------------------------------------------
	OLED_Init();							// OLED初始化
	OLED_ShowString(0,0,"ESP8266-Client");	// ESP8266-Client
	OLED_ShowString(0,2,"IP:");				// ESP8266_IP
	OLED_ShowString(0,4,"rationmcu.com");	// www.rationmcu.com
	OLED_ShowString(0,6,"IP:");				// 服务器_IP
	//------------------------------------------------------------

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
