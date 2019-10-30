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
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_esp_platform.h"
#include "user_iot_version.h"
#include "upgrade.h"

#if ESP_PLATFORM

#define ESP_DEBUG

#ifdef ESP_DEBUG
#define ESP_DBG os_printf	//【ESP_DBG】==【os_printf】
#else
#define ESP_DBG
#endif



// 修改设备数据点【技小新添加】
//-----------------------------------------------------------------------------------
//【"path"必选参数：数据点的"名称"】
//【"Authorization"必选参数：验证设备身份(Master Device Key)】
//【"datapoints"必选参数：维度(x、y、z、k、l)】
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define Modify_DATA_POINT	"{\"path\": \"/v1/datastreams/%s/datapoints/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"}, \"body\": {\"datapoints\":[{\"x\":%d}]}}\n"


//【云端设备】激活(类似于执行RESET操作)
//-------------------------------------------------------------------------------------------------------------
//【"nonce"可选参数：用于 MD5-NONCE，SHA1-NONCE，HMAC-SHA1-NONCE 加密算法，是一个随机的整数，用来混淆加密算法】
//【"encrypt_method"必要参数：激活方式(这里设为PLAIN方式)】
//【"token"可选参数：是40位随机字符，用于"拥有者授权"，在实际情况中可能是出厂说明书里面的“二维码”】
//【"bssid"可选参数：设置该设备的MAC地址】
//【"rom_version"可选参数：版本信息】
//【"Authorization"必选参数：验证设备身份(Master Device Key)】
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define ACTIVE_FRAME    "{\"nonce\": %d,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\"}}\n"

#if PLUG_DEVICE
#include "user_plug.h"

#define RESPONSE_FRAME  "{\"status\": 200, \"datapoint\": {\"x\": %d}, \"nonce\": %d, \"deliver_to_device\": true}\n"
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

#elif LIGHT_DEVICE
#include "user_light.h"


#define RESPONSE_FRAME  "{\"status\": 200,\"nonce\": %d, \"datapoint\": {\"x\": %d,\"y\": %d,\"z\": %d,\"k\": %d,\"l\": %d},\"deliver_to_device\":true}\n"

//【云下设备】向云平台标识身份，表明是设备发出的连接（ESP8266与乐鑫云平台建立连接后需向云平台标识身份，建立连接之后使用一次即可）
//-------------------------------------------------------------------------------------------------------------------------------
//【"nonce"可选参数：用于 MD5-NONCE，SHA1-NONCE，HMAC-SHA1-NONCE 加密算法，是一个随机的整数，用来混淆加密算法】
//【"Authorization"必选参数：验证设备身份(Master Device Key)】
//-----------------------------------------------------------------------------------------------------------------------------------------------
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

#elif SENSOR_DEVICE
#include "user_sensor.h"

#if HUMITURE_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %d, \"path\": \"/v1/datastreams/tem_hum/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %s%d.%02d,\"y\": %d.%02d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#elif FLAMMABLE_GAS_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %d, \"path\": \"/v1/datastreams/flammable_gas/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %d.%03d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#endif

LOCAL uint32 count = 0;
#endif

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"



#if PLUG_DEVICE || LIGHT_DEVICE

//【云下设备】定时和服务器(乐鑫云平台)保持心跳
//------------------------------------------------------------
//【"Authorization"必选参数：验证设备身份(Master Device Key)】
//-----------------------------------------------------------------------------------------------------------------------
#define BEACON_FRAME    "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n"


//【云下设备】向云平台应答(回答云平台的Rpc指令)
//-------------------------------------------------
//【"nonce"：返回云平台Rpc指令的"nonce"键对应的值】
//--------------------------------------------------------------------------------------------
#define RPC_RESPONSE_FRAME  "{\"status\": 200, \"nonce\": %d, \"deliver_to_device\": true}\n"


// 【云下设备】向服务器请求时间	【iot.espressif.cn(时区：中国北京时间)】	【iot.espressif.com(时区：格林尼治标准时间)】
//-----------------------------------------------------------------------------------------------------------------------
//【"Authorization"必选参数：验证设备身份(Master Device Key)】
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define TIMER_FRAME     "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n"

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

LOCAL uint8 ping_status;		// 心跳状态【1：ping success(心跳成功)	0：云平台未应答心跳包】
LOCAL os_timer_t beacon_timer;	// 心跳定时器【作用：保持与乐鑫云的连接，每50秒发送一次数据包(心跳包)】
#endif

#ifdef USE_DNS
ip_addr_t esp_server_ip;	// 服务器IP地址【乐鑫云】iot.espressif.cn == 115.29.202.58	iot.espressif.com == 119.9.91.208
#endif

LOCAL struct espconn user_conn;		// 网络连接结构体

LOCAL struct _esp_tcp user_tcp;		// TCP结构体

LOCAL os_timer_t client_timer;		// Client定时器(两个软件定时器之一)

struct esp_platform_saved_param esp_param;		//【云端/云下】设备的参数

LOCAL uint8 device_status;			//	设备状态：DEVICE_CONNECTING / DEVICE_ACTIVE_DONE / DEVICE_ACTIVE_FAIL / DEVICE_CONNECT_SERVER_FAIL

LOCAL uint8 device_recon_count = 0;		// ESP8266_TCP重连计数

LOCAL uint32 active_nonce = 0;			// 【激活加密码】激活时使用的算法加密整数码

LOCAL uint8 iot_version[20] = {0};		// 产品版本

struct rst_info rtc_info;		// RTC信息结构体

void user_esp_platform_check_ip(uint8 reset_flag);	// 函数声明

extern uint32 priv_param_start_sec;     // 0x7C(32MBit Flash)

/******************************************************************************
 * FunctionName : user_esp_platform_get_token
 * Description  : get the espressif's device token
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/

// 获取【云端设备】的esp_param.token
//====================================================================
void ICACHE_FLASH_ATTR user_esp_platform_get_token(uint8_t *token)
{
    if (token == NULL)
    {
        return;
    }

    // 读出esp_param.token
    //----------------------------------------------------------
    os_memcpy(token, esp_param.token, sizeof(esp_param.token));
}
//====================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_set_token
 * Description  : save the token for the espressif's device
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/

// 更新esp_param.token，将云端设备设为未激活状态【在JSON解析时，收到"token"调用】
//========================================================================================
void ICACHE_FLASH_ATTR user_esp_platform_set_token(uint8_t *token)
{
    if (token == NULL)	// 判断有无token指针
    {
        return;
    }

    // esp_param.activeflag ==【0】：云端设备未激活
    // esp_param.activeflag ==【1】：云端设备已激活
    esp_param.activeflag = 0;

    os_memcpy(esp_param.token, token, os_strlen(token));	// 更新esp_param.token

    // 保存esp_param到Flash
    //-----------------------------------------------------------------------------------
    system_param_save_with_protect(priv_param_start_sec+1,&esp_param,sizeof(esp_param));
}
//========================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_set_active
 * Description  : set active flag
 * Parameters   : activeflag -- 0 or 1
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_esp_platform_set_active(uint8 activeflag)
{
    esp_param.activeflag = activeflag;	// 设置esp_param.activeflag

    system_param_save_with_protect(priv_param_start_sec+1,&esp_param,sizeof(esp_param));	// 更新esp_param.token
}

void ICACHE_FLASH_ATTR
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_connect_status
 * Description  : get each connection step's status
 * Parameters   : none
 * Returns      : status
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    ESP_DBG("status %d\n", status);
    return status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_parse_nonce
 * Description  : parse the device nonce
 * Parameters   : pbuffer -- the recivce data point
 * Returns      : the nonce
*******************************************************************************/

// 获取【"nonce"键】对应的【"值"】
//==================================================================================
int ICACHE_FLASH_ATTR user_esp_platform_parse_nonce(char *pbuffer)
{
    char *pstr = NULL;
    char *pparse = NULL;
    char noncestr[11] = {0};
    int nonce = 0;
    pstr = (char *)os_strstr(pbuffer, "\"nonce\": ");	// 找到【"nonce": 】首地址

    if (pstr != NULL)
    {
        pstr += 9;
        pparse = (char *)os_strstr(pstr, ",");	// 找到【,】首地址

        if (pparse != NULL)
        {
            os_memcpy(noncestr, pstr, pparse-pstr);	// 获取【"nonce": 】对应的【"值"】
        }

        else	// pparse == NULL
        {
            pparse = (char *)os_strstr(pstr, "}");

            if (pparse != NULL) {
                os_memcpy(noncestr, pstr, pparse - pstr);
            } else {
                pparse = (char *)os_strstr(pstr, "]");

                if (pparse != NULL) {
                    os_memcpy(noncestr, pstr, pparse - pstr);
                } else {
                    return 0;
                }
            }
        }


        nonce = atoi(noncestr);		// 将【"值"(字符串)】转换为整型数据
    }

    return nonce;
}
//==================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_get_info
 * Description  : get and update the espressif's device status
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_esp_platform_get_info(struct espconn *pconn, uint8 *pbuffer)
{
    char *pbuf = NULL;
    int nonce = 0;

    pbuf = (char *)os_zalloc(packet_size);	// 开辟内存

    nonce = user_esp_platform_parse_nonce(pbuffer);	// 获取【"nonce"键】对应的【"值"】

    if (pbuf != NULL)
    {
#if PLUG_DEVICE
        os_sprintf(pbuf, RESPONSE_FRAME, user_plug_get_status(), nonce);
#elif LIGHT_DEVICE
        uint32 white_val;
        white_val = (PWM_CHANNEL>LIGHT_COLD_WHITE?user_light_get_duty(LIGHT_COLD_WHITE):0);
        os_sprintf(pbuf, RESPONSE_FRAME, nonce, user_light_get_period(),
                   user_light_get_duty(LIGHT_RED), user_light_get_duty(LIGHT_GREEN),
                   user_light_get_duty(LIGHT_BLUE),white_val );//50);
#endif

        ESP_DBG("%s\n", pbuf);
#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pconn, pbuf, os_strlen(pbuf));
#endif
        os_free(pbuf);
        pbuf = NULL;
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_info
 * Description  : prossing the data and controling the espressif's device
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_info(struct espconn *pconn, uint8 *pbuffer)
{
#if PLUG_DEVICE
    char *pstr = NULL;
    pstr = (char *)os_strstr(pbuffer, "plug-status");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "body");

        if (pstr != NULL) {

            if (os_strncmp(pstr + 27, "1", 1) == 0) {
                user_plug_set_status(0x01);
            } else if (os_strncmp(pstr + 27, "0", 1) == 0) {
                user_plug_set_status(0x00);
            }
        }
    }

#elif LIGHT_DEVICE
    char *pstr = NULL;
    char *pdata = NULL;
    char *pbuf = NULL;
    char recvbuf[10];
    uint16 length = 0;
    uint32 data = 0;
    static uint32 rr,gg,bb,cw,ww,period;
    ww=0;
    cw=0;
    extern uint8 light_sleep_flg;
    pstr = (char *)os_strstr(pbuffer, "\"path\": \"/v1/datastreams/light/datapoint/\"");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "{\"datapoint\": ");

        if (pstr != NULL) {
            pbuf = (char *)os_strstr(pbuffer, "}}");
            length = pbuf - pstr;
            length += 2;
            pdata = (char *)os_zalloc(length + 1);
            os_memcpy(pdata, pstr, length);

            pstr = (char *)os_strchr(pdata, 'x');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    period = data;
                    //user_light_set_period(data);
                }
            }

            pstr = (char *)os_strchr(pdata, 'y');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    rr=data;
                    os_printf("r: %d\r\n",rr);
                    //user_light_set_duty(data, 0);
                }
            }

            pstr = (char *)os_strchr(pdata, 'z');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    gg=data;
                    os_printf("g: %d\r\n",gg);
                    //user_light_set_duty(data, 1);
                }
            }

            pstr = (char *)os_strchr(pdata, 'k');

            if (pstr != NULL) {
                pstr += 4;;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    bb=data;
                    os_printf("b: %d\r\n",bb);
                    //user_light_set_duty(data, 2);
                }
            }

            pstr = (char *)os_strchr(pdata, 'l');

            if (pstr != NULL) {
                pstr += 4;;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    cw=data;
		      ww=data;
                    os_printf("cw: %d\r\n",cw);
		      os_printf("ww:%d\r\n",ww);   //chg
                    //user_light_set_duty(data, 2);
                }
            }

            os_free(pdata);
        }
    }
    
    if((rr|gg|bb|cw|ww) == 0){
        if(light_sleep_flg==0){

        }
        
    }else{
        if(light_sleep_flg==1){
            os_printf("modem sleep en\r\n");
            wifi_set_sleep_type(MODEM_SLEEP_T);
            light_sleep_flg =0;
        }
    }

    light_set_aim(rr,gg,bb,cw,ww,period);
    //user_light_restart();

#endif

    user_esp_platform_get_info(pconn, pbuffer);
}

/******************************************************************************
 * FunctionName : user_esp_platform_reconnect
 * Description  : reconnect with host after get ip
 * Parameters   : pespconn -- the espconn used to reconnect with host
 * Returns      : none
*******************************************************************************/

// TCP重连【检查IP、DNS解析、TCP连接设置】
//====================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_reconnect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_reconnect\n");

    user_esp_platform_check_ip(0);	// 查询ESP8266的IP获取情况
}
//====================================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_discon_cb
 * Description  : disconnect successfully with the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

// TCP连接断开成功【进行TCP重连】
//=================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_discon_cb(void *arg)
{
    struct espconn *pespconn = arg;
    struct ip_info ipconfig;
	struct dhcp_client_info dhcp_info;
    ESP_DBG("user_esp_platform_discon_cb\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);		// 关闭心跳定时器
#endif

    if (pespconn == NULL)	// 无网络连接
    {
        return;		// 返回
    }

    pespconn->proto.tcp->local_port = espconn_port();	// 获取本地端口

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
#endif

#if SENSOR_DEVICE
#ifdef SENSOR_DEEP_SLEEP

    if (wifi_get_opmode() == STATION_MODE) {
    	/***add by tzx for saving ip_info to avoid dhcp_client start****/
    	wifi_get_ip_info(STATION_IF, &ipconfig);

    	dhcp_info.ip_addr = ipconfig.ip;
    	dhcp_info.netmask = ipconfig.netmask;
    	dhcp_info.gw = ipconfig.gw ;
    	dhcp_info.flag = 0x01;
    	os_printf("dhcp_info.ip_addr = %d\n",dhcp_info.ip_addr);
    	system_rtc_mem_write(64,&dhcp_info,sizeof(struct dhcp_client_info));
        user_sensor_deep_sleep_enter();
    } else {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
        os_timer_arm(&client_timer, SENSOR_DEEP_SLEEP_TIME / 1000, 0);
    }

#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
#endif
#else
    user_esp_platform_reconnect(pespconn);		// TCP重连
#endif
}
//=================================================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_discon
 * Description  : A new incoming connection has been disconnected.
 * Parameters   : espconn -- the espconn used to disconnect with host
 * Returns      : none
*******************************************************************************/

// 断开TCP连接
//=================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_discon(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_discon\n");

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
#endif

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_disconnect(pespconn);
#else
    espconn_disconnect(pespconn);
#endif
}
//=================================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

// 网络数据发送成功_回调函数
//====================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_sent_cb\n");
}
//====================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/

// TCP连接建立后，ESP8266向乐鑫云平台发送TCP数据(激活、标识)
//=================================================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_sent(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};					// 【Device_Key】设备密钥
    uint32 nonce;
    char *pbuf = (char *)os_zalloc(packet_size);	// 申请数据包(2*1024Byte)空间

    os_memcpy(devkey, esp_param.devkey, 40);		// 获取esp_param.devkey值


	// esp_param.activeflag ==【0】：云端设备未激活【初始值==0xFF】
	// esp_param.activeflag ==【1】：云端设备已激活
	//------------------------------------------------------------------------------------------
    if(esp_param.activeflag == 0xFF)	// 如果 == 初始值 == 0xFF
    { esp_param.activeflag = 0; }		// 清0


    // 申请到了数据包空间(2*1024)
    //--------------------------------------------------------------------------
    if (pbuf != NULL)
    {
    	//【云端设备】未激活
    	//----------------------------------------------------------
        if (esp_param.activeflag == 0)	// 【0】：云端设备未激活
        {
            uint8 token[token_size] = {0};
            uint8 bssid[6];
            active_nonce = os_random()&0x7FFFFFFF;	// 第一次激活，获取随机的【激活加密码】

            os_memcpy(token, esp_param.token, 40);	// 获取esp_param.token值

            wifi_get_macaddr(STATION_IF, bssid);	// 获取STA的MAC地址


            // 格式化【ACTIVE_FRAME】，激活云端设备。【参数见"ACTIVE_FRAME"定义】
            //--------------------------------------------------------------------------------------
            os_sprintf(pbuf, ACTIVE_FRAME, active_nonce, token, MAC2STR(bssid),iot_version, devkey);
        }

#if SENSOR_DEVICE
#if HUMITURE_SUB_DEVICE
        else {
#if 0
            uint16 tp, rh;
            uint8 data[4];

            if (user_mvh3004_read_th(data)) {
                rh = data[0] << 8 | data[1];
                tp = data[2] << 8 | data[3];
            }

#else
            uint16 tp, rh;
            uint8 *data;
            uint32 tp_t, rh_t;
            data = (uint8 *)user_mvh3004_get_poweron_th();

            rh = data[0] << 8 | data[1];
            tp = data[2] << 8 | data[3];
#endif
            tp_t = (tp >> 2) * 165 * 100 / (16384 - 1);
            rh_t = (rh & 0x3fff) * 100 * 100 / (16384 - 1);

            if (tp_t >= 4000) {
                os_sprintf(pbuf, UPLOAD_FRAME, count, "", tp_t / 100 - 40, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            } else {
                tp_t = 4000 - tp_t;
                os_sprintf(pbuf, UPLOAD_FRAME, count, "-", tp_t / 100, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            }
        }

#elif FLAMMABLE_GAS_SUB_DEVICE
        else {
            uint32 adc_value = system_adc_read();

            os_sprintf(pbuf, UPLOAD_FRAME, count, adc_value / 1024, adc_value * 1000 / 1024, devkey);
        }

#endif
#else
        //【云端设备】已激活
        //----------------------------------------------------------
        else
        {
            nonce = os_random() & 0x7FFFFFFF;	// 获取随机的【加密码】

            //【云下设备】向服务器标识身份（建立连接之后使用一次即可）。【参数见"FIRST_FRAME"定义】
            //-------------------------------------------------------------------------------------
            os_sprintf(pbuf, FIRST_FRAME, nonce , devkey);
        }

#endif
        ESP_DBG("%s\n", pbuf);	// 串口打印 将要发送给乐鑫云服务器的【JSON字符串】

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));	// 发送数据包
#endif

        os_free(pbuf);	// 释放数据包空间
    }
}
//=================================================================================================================================

#if PLUG_DEVICE || LIGHT_DEVICE
/******************************************************************************
 * FunctionName : user_esp_platform_sent_beacon
 * Description  : sent beacon frame for connection with the host is activate
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/

// 发送心跳包_定时_回调函数
//==========================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_sent_beacon(struct espconn *pespconn)
{
    if (pespconn == NULL)
    {
        return;
    }

    if (pespconn->state == ESPCONN_CONNECT)
    {
        if (esp_param.activeflag == 0)	// 【云端设备】未激活
        {
            ESP_DBG("please check device is activated.\n");
            user_esp_platform_sent(pespconn);	// 激活【云端设备】
        }

        // 【云端设备】已激活
        //------------------------------------------------------------------
        else
        {
            uint8 devkey[token_size] = {0};

            os_memcpy(devkey, esp_param.devkey, 40);	//【Master Device Key】设备密钥

            ESP_DBG("user_esp_platform_sent_beacon %u\n", system_get_time());

            if (ping_status == 0)	// 心跳状态【1：ping success(心跳成功)	0：云平台未应答心跳包】
            {
                ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");

                user_esp_platform_discon(pespconn);		// 断开TCP连接
            }

            else
            {
                char *pbuf = (char *)os_zalloc(packet_size);	// 开辟数据包空间

                if (pbuf != NULL)
                {
                    os_sprintf(pbuf, BEACON_FRAME, devkey);		// 设置BEACON_FRAME格式(【云下设备】定时和服务器(乐鑫云平台)保持心跳)

#ifdef CLIENT_SSL_ENABLE
                    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
                    espconn_sent(pespconn, pbuf, os_strlen(pbuf));	// 发送数据包

                    ESP_DBG("%s\n", pbuf);	//
#endif

                    ping_status = 0;	// 心跳状态清0，等待乐鑫云的心跳应答

                    os_timer_arm(&beacon_timer, BEACON_TIME, 0);	// 心跳定时器 重新计时

                    os_free(pbuf);		// 释放数据包内存
                }
            }
        }
    }

    else
    {
        ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");

        user_esp_platform_discon(pespconn);		// 如果【云端设备】未激活，则断开TCP连接
    }
}
//==========================================================================================================


/******************************************************************************
 * FunctionName : user_platform_rpc_set_rsp
 * Description  : response the message to server to show setting info is received
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                nonce -- mark the message received from server
 * Returns      : none
*******************************************************************************/

//【云下设备】向云平台应答(回答云平台的Rpc指令)
//=====================================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_platform_rpc_set_rsp(struct espconn *pespconn, int nonce)
{
    char *pbuf = (char *)os_zalloc(packet_size);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, RPC_RESPONSE_FRAME, nonce);	//格式化【云下设备】向云平台应答(回答云平台的Rpc指令)的JSON字符串

    ESP_DBG("%s\n", pbuf);

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));	//【云下设备】向云平台应答(回答云平台的Rpc指令)
#endif
    os_free(pbuf);
}
//=====================================================================================================================

/******************************************************************************
 * FunctionName : user_platform_timer_get
 * Description  : get the timers from server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/

// 向服务器请求时间
//======================================================================================
LOCAL void ICACHE_FLASH_ATTR user_platform_timer_get(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};

    char *pbuf = (char *)os_zalloc(packet_size);	// 开辟数据包空间

    os_memcpy(devkey, esp_param.devkey, 40);	// 获取设备密钥【Master Device Key】

    if (pespconn == NULL)
    {
        return;
    }

    os_sprintf(pbuf, TIMER_FRAME, devkey);	// 格式化【向服务器请求时间】的JSON数据包

    ESP_DBG("%s\n", pbuf);	// 串口打印数据包

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));	// 向服务器请求时间
#endif
    os_free(pbuf);	// 释放内存
}
//======================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    struct espconn *pespconn = server->pespconn;
    uint8 devkey[41] = {0};
    uint8 *pbuf = NULL;
    char *action = NULL;

    os_memcpy(devkey, esp_param.devkey, 40);
    pbuf = (char *)os_zalloc(packet_size);

    if (server->upgrade_flag == true) {
        ESP_DBG("user_esp_platform_upgarde_successfully\n");
        action = "device_upgrade_success";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    } else {
        ESP_DBG("user_esp_platform_upgrade_failed\n");
        action = "device_upgrade_failed";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    }

    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_begin(struct espconn *pespconn, struct upgrade_server_info *server)
{
    uint8 user_bin[9] = {0};
    uint8 devkey[41] = {0};

    server->pespconn = pespconn;

    os_memcpy(devkey, esp_param.devkey, 40);
    os_memcpy(server->ip, pespconn->proto.tcp->remote_ip, 4);

#ifdef UPGRADE_SSL_ENABLE
    server->port = 443;
#else
    server->port = 80;
#endif

    server->check_cb = user_esp_platform_upgrade_rsp;
    server->check_times = 120000;

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(512);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, "user2.bin", 10);
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, "user1.bin", 10);
    }

    os_sprintf(server->url, "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
               server->upgrade_version, user_bin, IP2STR(server->ip),
               server->port, devkey);
    ESP_DBG("%s\n",server->url);

#ifdef UPGRADE_SSL_ENABLE

    if (system_upgrade_start_ssl(server) == false) {
#else

    if (system_upgrade_start(server) == false) {
#endif
        ESP_DBG("upgrade is already started\n");
    }
}

#endif		// #if PLUG_DEVICE || LIGHT_DEVICE


/******************************************************************************
 * FunctionName : user_esp_platform_recv_cb
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/

// 网络数据接收成功的回调函数
//============================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    char *pstr = NULL;
    LOCAL char pbuffer[1024 * 2] = {0};		// 开辟空间，缓存接收的网络数据
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_recv_cb:%s\n", pusrdata);	// 打印接收到的字符串
    ESP_DBG("\n-----------------------------------------------------------------------------------\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);		// 关闭心跳定时器
#endif


    // 接收的网络数据长度 == 1460
    //-----------------------------------------------------------------------------------------
    if (length == 1460)		// 明文报文的上限是1024字节，加密报文的上限是1460字节
    {
        os_memcpy(pbuffer, pusrdata, length);	// 将接收到的数据放入pbuffer
    }

    else	// 接收的网络数据长度 != 1460
    {
        struct espconn *pespconn = (struct espconn *)arg;

        os_memcpy(pbuffer+os_strlen(pbuffer), pusrdata, length);	// 将接收到的数据放入pbuffer[偏移0]


        // 判断接收到的消息是否包含【"activate_status": 】，并判断【激活加密码 active_nonce】是否相同
        //----------------------------------------------------------------------------------------------------------------------------------
        if ((pstr = (char *)os_strstr(pbuffer, "\"activate_status\": ")) != NULL && user_esp_platform_parse_nonce(pbuffer) == active_nonce)
        {
            if (os_strncmp(pstr+19,"1",1) == 0) 	// 判断激活状态值?=1
            {
                ESP_DBG("device activates successful.\n");

                device_status = DEVICE_ACTIVE_DONE;	// 云端设备激活成功

                esp_param.activeflag = 1;	// 【云端设备】激活状态 = 1

                system_param_save_with_protect(priv_param_start_sec+1, &esp_param, sizeof(esp_param));	// 将【云端设备】激活状态烧录到闪存中

                user_esp_platform_sent(pespconn);	// 发送FIRST_FRAME

                if(LIGHT_DEVICE)
                {
                    system_restart();	// 如果是Light，则复位
                }
            }

            else
            {
                ESP_DBG("device activates failed.\n");
                device_status = DEVICE_ACTIVE_FAIL;		// 云端设备激活失败
            }
        }


#if (PLUG_DEVICE || LIGHT_DEVICE)

        // 不是【云端设备】激活的回应信息
        //………………………………………………………………………………………………………………………………………………
        // 指令【"action": "sys_upgrade"】(通知设备需要更新)
        //-----------------------------------------------------------------------------------------------
        else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_upgrade\"")) != NULL)
        {
        	if ((pstr = (char *)os_strstr(pbuffer, "\"version\":")) != NULL) {
                struct upgrade_server_info *server = NULL;
                int nonce = user_esp_platform_parse_nonce(pbuffer);		// 获取【"nonce"键】对应的【"值"】
                user_platform_rpc_set_rsp(pespconn, nonce);		//【云下设备】向云平台应答(回答云平台的Rpc指令)

                server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
                os_memcpy(server->upgrade_version, pstr + 12, 16);
                server->upgrade_version[15] = '\0';
                os_sprintf(server->pre_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
                    	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
                user_esp_platform_upgrade_begin(pespconn, server);
            }
        }

        // 指令【"action": "sys_reboot"】(通知设备需要重启)
        //-------------------------------------------------------------------------------------
        else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_reboot\"")) != NULL)
        {
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
            os_timer_arm(&client_timer, 1000, 0);
        }

        else if ((pstr = (char *)os_strstr(pbuffer, "/v1/device/timers/")) != NULL)
        {
            int nonce = user_esp_platform_parse_nonce(pbuffer);		// 获取【"nonce"键】对应的【"值"】
            user_platform_rpc_set_rsp(pespconn, nonce);		//【云下设备】向云平台应答(回答云平台的Rpc指令)
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_platform_timer_get, pespconn);
            os_timer_arm(&client_timer, 2000, 0);
        }

        // 接收到【"method"】键
        //…………………………………………………………………………………………………………………………………………
        else if ((pstr = (char *)os_strstr(pbuffer, "\"method\": ")) != NULL)
        {
            if (os_strncmp(pstr + 11, "GET", 3) == 0)	// 判断【"值"】?= "GET"
            {
            	//####################################################################
            	//user_esp_platform_get_info(pespconn, pbuffer);	//【技小新注释掉】
            	//####################################################################

            	//【技小新添加的】
            	//##################################################################################################################
            	int nonce = user_esp_platform_parse_nonce(pbuffer);		// 获取【"nonce"键】对应的【"值"】
            	user_platform_rpc_set_rsp(pespconn, nonce);		//【云下设备】向云平台应答(回答云平台的Rpc指令)

            	if( ((char *)os_strstr(pbuffer, "{\"deliver_to_device\": true, \"get\": {\"action\": \"LED_ON\"}")) != NULL )
            	{
            		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);
            		GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED亮
            	}
            	else if( ((char *)os_strstr(pbuffer, "{\"deliver_to_device\": true, \"get\": {\"action\": \"LED_OFF\"}")) != NULL )
            	{
            		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);
            		GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// LED灭
            	}
            	//##################################################################################################################

            }
            else if (os_strncmp(pstr + 11, "POST", 4) == 0)	// 判断【"值"】?= "POST"
            {
            	//####################################################################
                //user_esp_platform_set_info(pespconn, pbuffer);	//【技小新注释掉】
                //####################################################################
            }
        }
        //…………………………………………………………………………………………………………………………………………

        // 心跳保持成功
        //--------------------------------------------------------------------------------------
        else if ((pstr = (char *)os_strstr(pbuffer, "ping success")) != NULL)
        {
            ESP_DBG("ping success\n");
            ping_status = 1;	// 心跳状态【1：ping success(心跳成功)	0：云平台未应答心跳包】
        }


        else if ((pstr = (char *)os_strstr(pbuffer, "send message success")) != NULL)
        {}	// 不执行任何操作


        // 当【云下设备】向服务器请求时间，乐鑫云回复，进入此处
        //-----------------------------------------------------------------
        else if ((pstr = (char *)os_strstr(pbuffer, "timers")) != NULL)
        {
            user_platform_timer_start(pusrdata , pespconn);
        }

#elif SENSOR_DEVICE
        else if ((pstr = (char *)os_strstr(pbuffer, "\"status\":")) != NULL) {
            if (os_strncmp(pstr + 10, "200", 3) != 0) {
                ESP_DBG("message upload failed.\n");
            } else {
                count++;
                ESP_DBG("message upload sucessful.\n");
            }

            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_discon, pespconn);
            os_timer_arm(&client_timer, 10, 0);
        }

#endif

        //当【云下设备】向服务器标识身份，乐鑫云回复，则进入此处
        //---------------------------------------------------------------
        else if ((pstr = (char *)os_strstr(pbuffer, "device")) != NULL)
        {
#if PLUG_DEVICE || LIGHT_DEVICE
            user_platform_timer_get(pespconn);	// 向服务器请求时间
#elif SENSOR_DEVICE

#endif
        }

        os_memset(pbuffer,0,sizeof(pbuffer));	// 将缓存网络数据的空间清0
    }

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_arm(&beacon_timer, BEACON_TIME, 0);	// 定时50秒(心跳定时器)
#endif
}


/******************************************************************************
 * FunctionName : user_esp_platform_ap_change
 * Description  : add the user interface for changing to next ap ID.
 * Parameters   :
 * Returns      : none
*******************************************************************************/

#if AP_CACHE

// (定时5秒)连接下一个记录的WIFI，并且检查IP获取情况
//=======================================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_ap_change(void)		// AP切换
{
    uint8 current_id;
    uint8 i = 0;
    ESP_DBG("user_esp_platform_ap_is_changing\n");

    current_id = wifi_station_get_current_ap_id();		// 获取当前连接的AP_id
    ESP_DBG("current ap id =%d\n", current_id);

    if (current_id == AP_CACHE_NUMBER - 1) 	// 0～4
    { i = 0; }
    else
    { i = current_id + 1; }

    while (wifi_station_ap_change(i) != true)	// 连接下一个AP
    {
       i++;
       if (i == AP_CACHE_NUMBER - 1) {
    	   i = 0;
       }
    }

    device_recon_count = 0;		// ESP8266_TCP重连计数 = 0

    /* just need to re-check ip while change AP */
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);		// 查询ESP8266的IP获取情况
    os_timer_arm(&client_timer, 100, 0);
}
#endif
//=======================================================================================================================

// ESP8266设为AP+STA模式，并且5秒切换WIFI
//==============================================================================================================
LOCAL bool ICACHE_FLASH_ATTR user_esp_platform_reset_mode(void)
{
    if (wifi_get_opmode() == STATION_MODE)	// ESP8266设为AP+STA模式
    {
        wifi_set_opmode(STATIONAP_MODE);	// 【开启AP模式使为了使用APP来向ESP8266发送路由器WIFI的SSID、PASS】
    }

#if AP_CACHE

    /* delay 5s to change AP */ // 5秒后连接下一个记录的WIFI
    //-----------------------------------------------------------------------------------
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_ap_change, NULL);
    os_timer_arm(&client_timer, 5000, 0);
    return true;

#endif

    return false;
}
//==============================================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_recon_cb
 * Description  : The connection had an error and is already deallocated.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

// 【ESP8266作为TCP_Client，连接TCP_Server失败 / TCP连接异常断开】的回调函数
//=================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

    ESP_DBG("user_esp_platform_recon_cb\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);		// 关闭心跳定时器
#endif

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
#endif

    if (++device_recon_count == 5)		/// ESP8266_TCP重连失败5次
    {
        device_status = DEVICE_CONNECT_SERVER_FAIL;		// 状态设为【设备连接服务器失败】

        if (user_esp_platform_reset_mode())		// ESP8266设为AP+STA模式，并且5秒切换WIFI
        {
            return;
        }
    }

#if SENSOR_DEVICE
#ifdef SENSOR_DEEP_SLEEP

    if (wifi_get_opmode() == STATION_MODE) {
        user_esp_platform_reset_mode();

        //user_sensor_deep_sleep_enter();
    } else {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
        os_timer_arm(&client_timer, 1000, 0);
    }

#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
#endif
#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);	// TCP重连【检查IP、DNS解析、TCP连接设置】
    os_timer_arm(&client_timer, 1000, 0);
#endif
}
//=================================================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

// TCP连接成功_回调函数
//==================================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_connect_cb\n");

    // 如果与服务器的TCP连接建立，那就关闭softAP模式
    //-----------------------------------------------
    if (wifi_get_opmode() ==  STATIONAP_MODE )	// 【开启AP模式使为了使用APP来向ESP8266发送路由器WIFI的SSID、PASS】
    {
        wifi_set_opmode(STATION_MODE);		// 设为STA模式
    }

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_timer_done();
#endif

    device_recon_count = 0;				// ESP8266_TCP重连计数 = 0

    espconn_regist_recvcb(pespconn, user_esp_platform_recv_cb);		// 网络数据接收成功_回调函数
    espconn_regist_sentcb(pespconn, user_esp_platform_sent_cb);		// 网络数据发送成功_回调函数

    user_esp_platform_sent(pespconn);	// ESP8266发送TCP数据（设备向乐鑫云平台发送数据包）
}
//==================================================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_connect
 * Description  : The function given as the connect with the host
 * Parameters   : espconn -- the espconn used to connect the connection
 * Returns      : none
*******************************************************************************/

// ESP8266(Client)连接TCP Server
//==================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_connect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_connect\n");		// 串口打印正在连接乐鑫云的字符串

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_connect(pespconn);
#else
    espconn_connect(pespconn);		// ESP8266(Client)连接TCP Server
#endif
}
//==================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/

#ifdef USE_DNS

// 域名解析成功的回调函数【TCP连接】
//============================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    // 域名解析错误【情形：成功连接WIFI，但是不能联网，所以解析域名失败】
    //---------------------------------------------------------------------
    if (ipaddr == NULL)
    {
        ESP_DBG("user_esp_platform_dns_found NULL\n");

        if (++device_recon_count == 5)	// 解析错误次数=5，则认为解析失败
        {
            device_status = DEVICE_CONNECT_SERVER_FAIL;

            user_esp_platform_reset_mode();		// ESP8266设为AP+STA模式，并且5秒切换WIFI
        }

        return;
    }

    // 串口打印乐鑫云平台的IP地址
    //-----------------------------------------------------------------
    ESP_DBG("user_esp_platform_dns_found %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr+1),
            *((uint8 *)&ipaddr->addr+2), *((uint8 *)&ipaddr->addr+3));

    // 成功获取乐鑫云的IP地址
    //-------------------------------------------------------------------------
    if (esp_server_ip.addr == 0 && ipaddr->addr != 0)
    {
        os_timer_disarm(&client_timer);		// Client定时器关闭

        esp_server_ip.addr = ipaddr->addr;								// IP地址赋值
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);	// 设置服务器地址
        pespconn->proto.tcp->local_port = espconn_port();				// 设置本地端口


#ifdef CLIENT_SSL_ENABLE
        pespconn->proto.tcp->remote_port = 8443;	// 加密Socket：		tcp://【iot.espressif.cn】【iot.espressif.com】:8443
#else
        pespconn->proto.tcp->remote_port = 8000;	// 不加密Socket：	tcp://【iot.espressif.cn】【iot.espressif.com】:8000
#endif

#if (PLUG_DEVICE || LIGHT_DEVICE)
        ping_status = 1;	// 心跳状态【1：ping success(心跳成功)	0：云平台未应答心跳包】
#endif

        espconn_regist_connectcb(pespconn, user_esp_platform_connect_cb);	// TCP连接成功_回调函数
        espconn_regist_disconcb(pespconn, user_esp_platform_discon_cb);		// TCP断开成功_回调函数
        espconn_regist_reconcb(pespconn, user_esp_platform_recon_cb);		// TCP异常断开_回调函数

        user_esp_platform_connect(pespconn);	// ESP8266(Client)连接TCP Server
    }
}
//============================================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

// DNS设置
//========================================================================================================================
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_dns_check_cb(void *arg)	// 定时检查域名是否解析成功
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_dns_check_cb\n");

    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);	// 域名解析

    os_timer_arm(&client_timer, 1000, 0);	// 定时查询域名解析情况
}

LOCAL void ICACHE_FLASH_ATTR user_esp_platform_start_dns(struct espconn *pespconn)
{
    esp_server_ip.addr = 0;
    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);	// 【乐鑫云平台】

    os_timer_disarm(&client_timer);		// 设置Client定时器
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_dns_check_cb, pespconn);	// 定时查询域名解析情况
    os_timer_arm(&client_timer, 1000, 0);
}
#endif
//========================================================================================================================

// mDNS设置
//=======================================================================================
#if LIGHT_DEVICE
void user_mdns_conf()
{
	struct ip_info ipconfig;

	wifi_get_ip_info(STATION_IF, &ipconfig);	// 获取STA模式下的IP地址

	struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
	info->host_name = "espressif_light_demo";
	info->ipAddr= ipconfig.ip.addr; //sation ip
	info->server_name = "espLight";
	info->server_port = 80;
	info->txt_data[0] = "version = 1.0.1";
	espconn_mdns_init(info);					// 初始化mDNS
}
#endif
//=======================================================================================

/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
// 查询ESP8266的IP获取情况【DNS解析、TCP设置】
//==================================================================================================================================
void ICACHE_FLASH_ATTR user_esp_platform_check_ip(uint8 reset_flag)
{
    struct ip_info ipconfig;					// IP信息结构体

    os_timer_disarm(&client_timer);				// 暂时关闭Client定时器

    wifi_get_ip_info(STATION_IF, &ipconfig);	// 获取STA模式下的IP地址

    // ESP8266获取到IP地址
    //---------------------------------------------------------------------------
    if (wifi_station_get_connect_status()==STATION_GOT_IP && ipconfig.ip.addr!=0)
    {
#if (PLUG_DEVICE || SENSOR_DEVICE)
        user_link_led_timer_init();
#endif

//--------------------------------------
#if LIGHT_DEVICE
	user_mdns_conf();	// mDNS初始化
#endif
//--------------------------------------

		// TCP连接设置
		//------------------------------------------------------------------
        user_conn.proto.tcp = &user_tcp;	// 开辟内存
        user_conn.type = ESPCONN_TCP;		// 设为TCP连接
        user_conn.state = ESPCONN_NONE;

        device_status = DEVICE_CONNECTING;	// 设备(8266)状态设为【正在连接】

        if (reset_flag)				// 判断是否是刚复位
        { device_recon_count = 0;}	// 系统复位后，ESP8266_TCP重连计数=0（reset_flag==1表示为系统复位）

#if (PLUG_DEVICE || LIGHT_DEVICE)
        os_timer_disarm(&beacon_timer);	// 设置心跳定时器定时器
        os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
#endif

#ifdef USE_DNS
        user_esp_platform_start_dns(&user_conn);	// DNS设置
#else
        const char esp_server_ip[4] = {114, 215, 177, 97};

        os_memcpy(user_conn.proto.tcp->remote_ip, esp_server_ip, 4);
        user_conn.proto.tcp->local_port = espconn_port();

#ifdef CLIENT_SSL_ENABLE
        user_conn.proto.tcp->remote_port = 8443;
#else
        user_conn.proto.tcp->remote_port = 8000;
#endif

        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        user_esp_platform_connect(&user_conn);
#endif
    }

    // 未获取到IP地址
    //------------------------------------------------------------------------------------------
    else
    {
        /* if there are wrong while connecting to some AP, then reset mode */
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||		// [密码错误]、[未发现AP]、[连接失败]
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL))
        {
            user_esp_platform_reset_mode();		// ESP8266设为AP+STA模式，并且5秒切换WIFI
        }
        else
        {
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);	// 继续查询ESP8266的IP获取情况
            os_timer_arm(&client_timer, 100, 0);
        }
    }
}
//==================================================================================================================================


/******************************************************************************
 * FunctionName : user_esp_platform_init
 * Description  : device parame init based on espressif platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
// 查询复位状态、参数初始化
//==================================================================================================================================
void ICACHE_FLASH_ATTR user_esp_platform_init(void)
{
	// 获取版本信息
	//--------------------------------------------------------------------------
	os_sprintf(iot_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
	os_printf("IOT VERSION = %s\n",iot_version);

	//--------------------------------------------------------------------------------------------------------------------
	system_param_load(priv_param_start_sec+1,0,&esp_param,sizeof(esp_param));	// 读取【0x7D(0x7C+1)扇区】的数据(KEY_BIN)
	os_printf("esp_param.devkey = %s\n",esp_param.devkey);						// 串口打印【devkey】
	os_printf("esp_param.token = %s\n",esp_param.token);						// 串口打印【token】
	os_printf("esp_param.pad = %s\n",esp_param.pad);							// 串口打印【pad】
	os_printf("esp_param.activeflag = %d\n",esp_param.activeflag);				// 串口打印【activeflag】

//	while(1) system_soft_wdt_feed();	// 【技小新添加】

	// ESP8266复位后，执行复位查询
	//-------------------------------------------------------------------------
	struct rst_info *rtc_info = system_get_rst_info();	// 获取当前的启动信息
	os_printf("reset reason: %x\n", rtc_info->reason);	// 打印复位原因

	// 判断复位原因
	//--------------------------------------------------------------------
	if (rtc_info->reason == REASON_WDT_RST ||			// 看门狗复位
		rtc_info->reason == REASON_EXCEPTION_RST ||		// 异常复位
		rtc_info->reason == REASON_SOFT_WDT_RST)		// 软件看门狗复位
	{
		if (rtc_info->reason == REASON_EXCEPTION_RST)
		{
			os_printf("Fatal exception (%d):\n", rtc_info->exccause);
		}
		os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
		rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
	}


	// 保存之前的IP地址
	//-------------------------------------------------------------------------
	/***add by tzx for saving ip_info to avoid dhcp_client start****/
    struct dhcp_client_info dhcp_info;
    struct ip_info sta_info;
    system_rtc_mem_read(64,&dhcp_info,sizeof(struct dhcp_client_info));	// 读取RTC memory中的数据

    // 判断之前是否保存为1
    //-----------------------------------------------------------------
	if(dhcp_info.flag == 0x01 )
	{
		if (true == wifi_station_dhcpc_status())	// STA_DHCP启动
		{
			wifi_station_dhcpc_stop();				// STA_DHCP停止
		}

		sta_info.ip = dhcp_info.ip_addr;	// 重新设为之前的IP地址
		sta_info.gw = dhcp_info.gw;
		sta_info.netmask = dhcp_info.netmask;
		if ( true != wifi_set_ip_info(STATION_IF,&sta_info))	// 设置STA的IP地址
		{ os_printf("set default ip wrong\n"); }
	}

    os_memset(&dhcp_info,0,sizeof(struct dhcp_client_info));	 // dhcp_info清0
    system_rtc_mem_write(64,&dhcp_info,sizeof(struct rst_info));// RTC_mem清0


#if AP_CACHE
    wifi_station_ap_number_set(AP_CACHE_NUMBER);	// AP信息缓存(5个)
#endif


#if 0
    {
        char sofap_mac[6] = {0x16, 0x34, 0x56, 0x78, 0x90, 0xab};
        char sta_mac[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab};
        struct ip_info info;

        wifi_set_macaddr(SOFTAP_IF, sofap_mac);
        wifi_set_macaddr(STATION_IF, sta_mac);

        IP4_ADDR(&info.ip, 192, 168, 3, 200);
        IP4_ADDR(&info.gw, 192, 168, 3, 1);
        IP4_ADDR(&info.netmask, 255, 255, 255, 0);
        wifi_set_ip_info(STATION_IF, &info);

        IP4_ADDR(&info.ip, 10, 10, 10, 1);
        IP4_ADDR(&info.gw, 10, 10, 10, 1);
        IP4_ADDR(&info.netmask, 255, 255, 255, 0);
        wifi_set_ip_info(SOFTAP_IF, &info);
    }
#endif


    // esp_param.activeflag ==【0】：云端设备未激活【初始值==0xFF】
    // esp_param.activeflag ==【1】：云端设备已激活
    //------------------------------------------------------------------
    if (esp_param.activeflag != 1)		// 【云端设备】未激活
    {
#ifdef SOFTAP_ENCRYPT
        struct softap_config config;
        char password[33];
        char macaddr[6];

        wifi_softap_get_config(&config);
        wifi_get_macaddr(SOFTAP_IF, macaddr);

        os_memset(config.password, 0, sizeof(config.password));
        os_sprintf(password, MACSTR "_%s", MAC2STR(macaddr), PASSWORD);
        os_memcpy(config.password, password, os_strlen(password));
        config.authmode = AUTH_WPA_WPA2_PSK;

        wifi_softap_set_config(&config);
#endif

        wifi_set_opmode(STATIONAP_MODE);	// 设为AP+STA模式【开启AP模式使为了使用APP来向ESP8266发送路由器WIFI的SSID、PASS】
    }

#if PLUG_DEVICE
    user_plug_init();	// 插座初始化
#elif LIGHT_DEVICE
    user_light_init();	// 灯光初始化(PWM)
#elif SENSOR_DEVICE
    user_sensor_init(esp_param.activeflag);	// 传感器初始化
#endif

    // 判断ESP8266是否为SoftAP模式
    //-------------------------------------------------------------------------------------------
    if (wifi_get_opmode() != SOFTAP_MODE)	// 不是SoftAP模式
    {
    	// 设置定时器（定时查询ESP8266的IP情况）
    	//----------------------------------------
    	os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip,(void *)1);	// 参数3=1：表示当前是刚复位状态
        os_timer_arm(&client_timer, 100, 0);	// 使能毫秒定时器(100Ms定时)
    }
}
//==================================================================================================================================

#endif
