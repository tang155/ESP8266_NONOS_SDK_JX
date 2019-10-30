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

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_json.h"
#include "user_devicefind.h"

const char *device_find_request = "Are You Espressif IOT Smart Device?";
#if PLUG_DEVICE
const char *device_find_response_ok = "I'm Plug.";
#elif LIGHT_DEVICE
const char *device_find_response_ok = "I'm Light.";
#elif SENSOR_DEVICE
#if HUMITURE_SUB_DEVICE
const char *device_find_response_ok = "I'm Humiture.";
#elif FLAMMABLE_GAS_SUB_DEVICE
const char *device_find_response_ok = "I'm Flammable Gas.";
#endif
#endif

/*---------------------------------------------------------------------------*/
LOCAL struct espconn ptrespconn;

/******************************************************************************
 * FunctionName : user_devicefind_recv
 * Description  : Processing the received data from the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR user_devicefind_recv(void *arg, char *pusrdata, unsigned short length)
{
    char DeviceBuffer[40] = {0};
    char Device_mac_buffer[60] = {0};
    char hwaddr[6];
    remot_info *premot = NULL;
    struct ip_info ipconfig;

    // ESP8266不是STA模式
    //-------------------------------------------------------------------------------------------------------------
    if (wifi_get_opmode() != STATION_MODE)
    {
        wifi_get_ip_info(SOFTAP_IF, &ipconfig);		// 获取AP模式下的IP地址
        wifi_get_macaddr(SOFTAP_IF, hwaddr);		// 获取AP模式下的MAC地址

        // ip_addr_netcmp!=0，表示两个地址在同一个网络地址段中
        //--------------------------------------------------------------------------------------------------------
        if (! ip_addr_netcmp((struct ip_addr *)ptrespconn.proto.udp->remote_ip, &ipconfig.ip, &ipconfig.netmask))
        {
        	// 两个地址不在同一个地址段中
        	//-------------------------------------------------------------------
            wifi_get_ip_info(STATION_IF, &ipconfig);	// 获取STA模式下的IP地址
            wifi_get_macaddr(STATION_IF, hwaddr);		// 获取STA模式下的MAC地址
        }
    }

    // ESP8266是STA模式
    //------------------------------------------------------------------------
    else
    {
        wifi_get_ip_info(STATION_IF, &ipconfig);	// 获取STA模式下的IP地址
        wifi_get_macaddr(STATION_IF, hwaddr);		// 获取STA模式下的MAC地址
    }

    if (pusrdata == NULL) 	// 接收数据出错
    {
        return;
    }

    // 接收到的数据 == "Are You Espressif IOT Smart Device?"
    //------------------------------------------------------------------------------------------------------------------------------
    if (length == os_strlen(device_find_request) && os_strncmp(pusrdata, device_find_request, os_strlen(device_find_request)) == 0)
    {
    	// 将"I'm Light." "ESP8266MAC地址" "IP地址"，按照规定的格式，依次写入DeviceBuffer数组(自我介绍字符串)
    	//---------------------------------------------------------------------------------------------------------------
        os_sprintf(DeviceBuffer, "%s" MACSTR " " IPSTR, device_find_response_ok, MAC2STR(hwaddr), IP2STR(&ipconfig.ip));
        os_printf("%s\n", DeviceBuffer);

        length = os_strlen(DeviceBuffer);	// 计算"自我介绍"字符串总长度

        if (espconn_get_connection_info(&ptrespconn, &premot, 0) != ESPCONN_OK)		// 获取远端信息
        	return;

        os_memcpy(ptrespconn.proto.udp->remote_ip, premot->remote_ip, 4);	// 获取远端IP地址
        ptrespconn.proto.udp->remote_port = premot->remote_port;			// 获取远端端口
        espconn_sent(&ptrespconn, DeviceBuffer, length);					// 向对方应答"自我介绍"字符串
    }

    // 接收到的数据 == 53字节
    //------------------------------------------------------------------------------------------
    else if (length == (os_strlen(device_find_request) + 18))
    {
    	// 将"Are You Espressif IOT Smart Device?" "ESP8266MAC地址"，按照规定的格式，依次写入Device_mac_buffer数组
    	//---------------------------------------------------------------------------------------------------------
        os_sprintf(Device_mac_buffer, "%s " MACSTR , device_find_request, MAC2STR(hwaddr));
        os_printf("%s", Device_mac_buffer);

        // 接收到的数据 ==【"Are You Espressif IOT Smart Device?" + " " + MACSTR】
        //-------------------------------------------------------------------------------------
        if (os_strncmp(Device_mac_buffer, pusrdata, os_strlen(device_find_request) + 18) == 0)
        {
            //os_printf("%s\n", Device_mac_buffer);

            length = os_strlen(DeviceBuffer);

            // 将"I'm Light." "ESP8266MAC地址" "IP地址"，按照规定的格式，依次写入DeviceBuffer数组(自我介绍字符串)
            //---------------------------------------------------------------------------------------------------------------
            os_sprintf(DeviceBuffer, "%s" MACSTR " " IPSTR, device_find_response_ok, MAC2STR(hwaddr), IP2STR(&ipconfig.ip));
            os_printf("%s\n", DeviceBuffer);

            length = os_strlen(DeviceBuffer);	// 计算"自我介绍"字符串总长度


			if (espconn_get_connection_info(&ptrespconn, &premot, 0) != ESPCONN_OK)		// 获取远端信息
				return;

			os_memcpy(ptrespconn.proto.udp->remote_ip, premot->remote_ip, 4);	// 获取远端IP地址
			ptrespconn.proto.udp->remote_port = premot->remote_port;			// 获取远端端口
            espconn_sent(&ptrespconn, DeviceBuffer, length);					// 向对方应答"自我介绍"字符串
        }

        else	// 网络数据接收错误，返回
        {
            return;
        }
    }
}

/******************************************************************************
 * FunctionName : user_devicefind_init
 * Description  : the espconn struct parame init
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_devicefind_init(void)
{
    ptrespconn.type = ESPCONN_UDP;			// 初始化为UDP通信
    ptrespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));	// 分配空间
    ptrespconn.proto.udp->local_port = 1025;						// 本地端口
    espconn_regist_recvcb(&ptrespconn, user_devicefind_recv);	// 注册成功接收网络数据的回调函数
    espconn_create(&ptrespconn);	// 创建UDP通信
}
