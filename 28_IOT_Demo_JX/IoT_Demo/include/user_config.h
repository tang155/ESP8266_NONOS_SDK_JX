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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ESP_PLATFORM		1	// 使能ESP平台(乐鑫)
#define LEWEI_PLATFORM		0

#define USE_OPTIMIZE_PRINTF

//乐鑫云平台下的设备选择
//----------------------------------------
#if ESP_PLATFORM
#define PLUG_DEVICE			0	// 插座
#define LIGHT_DEVICE		1	// 灯光
#define SENSOR_DEVICE		0	// 传感器


// 传感器下的设备选择
//--------------------------------------------------------
#if SENSOR_DEVICE
#define HUMITURE_SUB_DEVICE         1	// 温湿度传感器
#define FLAMMABLE_GAS_SUB_DEVICE    0	// 易燃气体传感器
#endif


//#define SERVER_SSL_ENABLE		// 服务器SSL使能
//#define CLIENT_SSL_ENABLE		// 客户端SSL使能
//#define UPGRADE_SSL_ENABLE	// 升级SSL使能


// DNS设置
//-------------------------------------------------------------------------------------------------------
#define USE_DNS		// 使用DNS

#ifdef  USE_DNS

#define ESP_DOMAIN		"iot.espressif.cn"		// 115.29.202.58【iot.espressif.cn(时区：中国北京时间)】
//#define ESP_DOMAIN	"iot.espressif.com"		// 119.9.91.208【iot.espressif.com(时区：格林尼治标准时间)】

#endif
//-----------------------------------------------------------------


// softAP译码设置
//-------------------------------------
//#define SOFTAP_ENCRYPT

#ifdef SOFTAP_ENCRYPT
#define PASSWORD	"v*%W>L<@i&Nxe!"
#endif


// 传感器睡眠设置
//-------------------------------------------
#if SENSOR_DEVICE
#define SENSOR_DEEP_SLEEP

#if HUMITURE_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    30000000
#elif FLAMMABLE_GAS_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    60000000
#endif
#endif


// 灯光设置
//--------------------------------------
#if LIGHT_DEVICE
#define USE_US_TIMER	// 微秒定时(us)
#endif

// 灯光/插座设置
//-------------------------------------
#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_TIMEOUT  150000000
#define BEACON_TIME     50000
#endif

// AP缓存
//--------------------------------------------
#define AP_CACHE           1	// 使能AP缓存

#if AP_CACHE
#define AP_CACHE_NUMBER    5	// 最多缓存5个
#endif

// 如果不是乐鑫云平台
//---------------------
#elif LEWEI_PLATFORM
#endif


#endif	/*__USER_CONFIG_H__*/

