/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														//																		//
// ���̣�	MQTT_Data-Visualization						//	ע���ڡ�MQTT_JX���������޸�											//
//														//																		//
// ƽ̨��	�����µ��ӡ������������� ESP8266 V1.0		//	�٣����մ�������Ӱ�ӡ�ʱ��ȡ����MQTT���Ӳ������޸�<mqtt_config.h>	//
//														//																		//
// ���ܣ�	�٣����á���Ӱ�ӡ�MQTT��ز���				//	�ڣ���<dht11.c>��ӵ�<modules>����<dht11.h>��ӵ�<include/modules>	//
//														//																		//
//			�ڣ�ESP8266����ٶ��ơ���Ӱ�ӡ�				//	�ۣ���ע�⣬<dht11.c>�У����Ϊ<#include modules/dht11.h>			//
//														//																		//
//			�ۣ�ÿ5������Ӱ�ӡ��ϱ�����ʪ�����ݡ�		//	�ܣ���<mqtt.c>��ʱ�����У���ӣ�ÿ5������Ӱ�ӡ��ϱ�����ʪ�����ݡ�	//
//														//																		//
//	�汾��	V1.0										//	�ݣ�ȡ��������"SW_LED"������"SW_LED"������Ϣ������					//
//														//																		//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// ͷ�ļ�
//==============================
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"

#include "dht11.h"	// DHT11
#include "driver/oled.h"  		// OLED

//==============================

// ���Ͷ���
//=================================
typedef unsigned long 		u32_t;
//=================================


// ȫ�ֱ���
//============================================================================
MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��

static ETSTimer sntp_timer;		// SNTP��ʱ��
//============================================================================

// ������ʱ����
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
// SNTP��ʱ�ص�����
//===================================================================================================
u8 C_Read_DHT11 = 0;				// ��ȡDHT11��ʱ
void ICACHE_FLASH_ATTR OS_Timer_SNTP_cb(void	 * arg)
{
	// �ַ������� ��ر���
	//------------------------------------------------------

	u8 C_Str = 0;				// �ַ����ֽڼ���

	char A_Str_Data[20] = {0};	// ��"����"���ַ�������

	char *T_A_Str_Data = A_Str_Data;	// ��������ָ��

	char A_Str_Clock[10] = {0};	// ��"ʱ��"���ַ�������


	char * Str_Head_Week;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Month;		// ��"�·�"���ַ����׵�ַ

	char * Str_Head_Day;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Clock;		// ��"ʱ��"���ַ����׵�ַ

	char * Str_Head_Year;		// ��"���"���ַ����׵�ַ

	//------------------------------------------------------


	 uint32	TimeStamp;		// ʱ���

	 char * Str_RealTime;	// ʵ��ʱ����ַ���


	 // ��ѯ��ǰ�����׼ʱ��(1970.01.01 00:00:00 GMT+8)��ʱ���(��λ:��)
	 //-----------------------------------------------------------------
	 TimeStamp = sntp_get_current_timestamp();

	 if(TimeStamp)		// �ж��Ƿ��ȡ��ƫ��ʱ��
	 {
		 //os_timer_disarm(&OS_Timer_SNTP);	// �ر�SNTP��ʱ��

		 // ��ѯʵ��ʱ��(GMT+8):������(����ʱ��)
		 //--------------------------------------------
		 Str_RealTime = sntp_get_real_time(TimeStamp);


		 // ��ʵ��ʱ�䡿�ַ��� == "�� �� �� ʱ:��:�� ��"
		 //------------------------------------------------------------------------
		 os_printf("\r\n----------------------------------------------------\r\n");
		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// ʱ���
		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// ʵ��ʱ��
		 os_printf("--------------------------------------------------------\r\n");


		 // ʱ���ַ�������OLED��ʾ��"����"������"ʱ��"���ַ���
		 //��������������������������������������������������������������������������������������

		 // ��"���" + ' '��������������
		 //---------------------------------------------------------------------------------
		 Str_Head_Year = Str_RealTime;	// ������ʼ��ַ

		 while( *Str_Head_Year )		// �ҵ���"ʵ��ʱ��"���ַ����Ľ����ַ�'\0'
			 Str_Head_Year ++ ;

		 // ��ע��API���ص�ʵ��ʱ���ַ����������һ�����з����������� -5��
		 //-----------------------------------------------------------------
		 Str_Head_Year -= 5 ;			// ��ȡ��"���"���ַ������׵�ַ

		 T_A_Str_Data[4] = ' ' ;
		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// ��"���" + ' '��������������

		 T_A_Str_Data += 5;				// ָ��"���" + ' '���ַ����ĺ���ĵ�ַ
		 //---------------------------------------------------------------------------------

		 // ��ȡ�����ڡ��ַ������׵�ַ
		 //---------------------------------------------------------------------------------
		 Str_Head_Week 	= Str_RealTime;							// "����" �ַ������׵�ַ
		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "�·�" �ַ������׵�ַ
		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "����" �ַ������׵�ַ
		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "ʱ��" �ַ������׵�ַ


		 // ��"�·�" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Day - Str_Head_Month;				// ��"�·�" + ' '�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// ��"�·�" + ' '��������������

		 T_A_Str_Data += C_Str;		// ָ��"�·�" + ' '���ַ����ĺ���ĵ�ַ


		 // ��"����" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Clock - Str_Head_Day;				// ��"����" + ' '�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// ��"����" + ' '��������������

		 T_A_Str_Data += C_Str;		// ָ��"����" + ' '���ַ����ĺ���ĵ�ַ


		 // ��"����" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// ��"����"�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// ��"����"��������������

		 T_A_Str_Data += C_Str;		// ָ��"����"���ַ����ĺ���ĵ�ַ


		 // OLED��ʾ��"����"������"ʱ��"���ַ���
		 //---------------------------------------------------------------------------------
		 *T_A_Str_Data = '\0';		// ��"����"���ַ����������'\0'

		 OLED_ShowString(0,0,A_Str_Data);		// OLED��ʾ����


		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// ��"ʱ��"���ַ�������ʱ������
		 A_Str_Clock[8] = '\0';

		 OLED_ShowString(64,2,A_Str_Clock);		// OLED��ʾʱ��

		 //��������������������������������������������������������������������������������������
	 }


	// ÿ5�룬��ȡ/��ʾ��ʪ������
	//-----------------------------------------------------------------------------------------
	C_Read_DHT11 ++ ;		// ��ȡDHT11��ʱ

	if(C_Read_DHT11>=5)		// 5���ʱ
	{
		C_Read_DHT11 = 0;	// ��ʱ=0

		if(DHT11_Read_Data_Complete() == 0)		// ��ȡDHT11��ʪ��
		{
			DHT11_NUM_Char();	// DHT11����ֵת���ַ���

			OLED_ShowString(64,4,DHT11_Data_Char[1]);	// DHT11_Data_Char[0] == ���¶��ַ�����
			OLED_ShowString(64,6,DHT11_Data_Char[0]);	// DHT11_Data_Char[1] == ��ʪ���ַ�����
		}

		else
		{
    		OLED_ShowString(64,4,"----");	// Temperature���¶�
    		OLED_ShowString(64,6,"----");	// Humidity��ʪ��
		}
	}
	//-----------------------------------------------------------------------------------------
}
// SNTP��ʱ��������ȡ��ǰ����ʱ��
//============================================================================
void sntpfn()
{
    u32_t ts = 0;

	// �ַ������� ��ر���
	//------------------------------------------------------

	u8 C_Str = 0;				// �ַ����ֽڼ���

	char A_Str_Data[20] = {0};	// ��"����"���ַ�������

	char *T_A_Str_Data = A_Str_Data;	// ��������ָ��

	char A_Str_Clock[10] = {0};	// ��"ʱ��"���ַ�������


	char * Str_Head_Week;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Month;		// ��"�·�"���ַ����׵�ַ

	char * Str_Head_Day;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Clock;		// ��"ʱ��"���ַ����׵�ַ

	char * Str_Head_Year;		// ��"���"���ַ����׵�ַ

	//------------------------------------------------------


	 uint32	TimeStamp;		// ʱ���

	 char * Str_RealTime;	// ʵ��ʱ����ַ���


	 // ��ѯ��ǰ�����׼ʱ��(1970.01.01 00:00:00 GMT+8)��ʱ���(��λ:��)
	 //-----------------------------------------------------------------
	 TimeStamp = sntp_get_current_timestamp();

	 if(TimeStamp)		// �ж��Ƿ��ȡ��ƫ��ʱ��
	 {
		 //os_timer_disarm(&OS_Timer_SNTP);	// �ر�SNTP��ʱ��

		 // ��ѯʵ��ʱ��(GMT+8):������(����ʱ��)
		 //--------------------------------------------
		 Str_RealTime = sntp_get_real_time(TimeStamp);


		 // ��ʵ��ʱ�䡿�ַ��� == "�� �� �� ʱ:��:�� ��"
		 //------------------------------------------------------------------------
		 os_printf("\r\n----------------------------------------------------\r\n");
		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// ʱ���
		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// ʵ��ʱ��
		 os_printf("--------------------------------------------------------\r\n");


		 // ʱ���ַ�������OLED��ʾ��"����"������"ʱ��"���ַ���
		 //��������������������������������������������������������������������������������������

		 // ��"���" + ' '��������������
		 //---------------------------------------------------------------------------------
		 Str_Head_Year = Str_RealTime;	// ������ʼ��ַ

		 while( *Str_Head_Year )		// �ҵ���"ʵ��ʱ��"���ַ����Ľ����ַ�'\0'
			 Str_Head_Year ++ ;

		 // ��ע��API���ص�ʵ��ʱ���ַ����������һ�����з����������� -5��
		 //-----------------------------------------------------------------
		 Str_Head_Year -= 5 ;			// ��ȡ��"���"���ַ������׵�ַ

		 T_A_Str_Data[4] = ' ' ;
		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// ��"���" + ' '��������������

		 T_A_Str_Data += 5;				// ָ��"���" + ' '���ַ����ĺ���ĵ�ַ
		 //---------------------------------------------------------------------------------

		 // ��ȡ�����ڡ��ַ������׵�ַ
		 //---------------------------------------------------------------------------------
		 Str_Head_Week 	= Str_RealTime;							// "����" �ַ������׵�ַ
		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "�·�" �ַ������׵�ַ
		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "����" �ַ������׵�ַ
		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "ʱ��" �ַ������׵�ַ


		 // ��"�·�" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Day - Str_Head_Month;				// ��"�·�" + ' '�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// ��"�·�" + ' '��������������

		 T_A_Str_Data += C_Str;		// ָ��"�·�" + ' '���ַ����ĺ���ĵ�ַ


		 // ��"����" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Clock - Str_Head_Day;				// ��"����" + ' '�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// ��"����" + ' '��������������

		 T_A_Str_Data += C_Str;		// ָ��"����" + ' '���ַ����ĺ���ĵ�ַ


		 // ��"����" + ' '��������������
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// ��"����"�����ֽ���

		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// ��"����"��������������

		 T_A_Str_Data += C_Str;		// ָ��"����"���ַ����ĺ���ĵ�ַ


		 // OLED��ʾ��"����"������"ʱ��"���ַ���
		 //---------------------------------------------------------------------------------
		 *T_A_Str_Data = '\0';		// ��"����"���ַ����������'\0'

		 OLED_ShowString(0,0,A_Str_Data);		// OLED��ʾ����


		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// ��"ʱ��"���ַ�������ʱ������
		 A_Str_Clock[8] = '\0';

		 OLED_ShowString(64,2,A_Str_Clock);		// OLED��ʾʱ��

		 //��������������������������������������������������������������������������������������
	 }

//    ts = sntp_get_current_timestamp();		// ��ȡ��ǰ��ƫ��ʱ��

    os_printf("current time : %s\n", sntp_get_real_time(TimeStamp));	// ��ȡ��ʵʱ��

    if (TimeStamp == 0)		// ����ʱ���ȡʧ��
    {
        os_printf("did not get a valid time from sntp server\n");
    }
    else //(ts != 0)	// ����ʱ���ȡ�ɹ�
    {
//		 	 OLED_ShowString(0,0,sntp_get_real_time(TimeStamp));		// OLED��ʾ����
//			 OLED_ShowString(64,2,sntp_get_real_time(TimeStamp));		// OLED��ʾʱ��


//            os_timer_disarm(&sntp_timer);	// �ر�SNTP��ʱ��
            static uint8 mqtt_start=0;
            if(mqtt_start == 0)
            {
                MQTT_Connect(&mqttClient);		// ��ʼMQTT����
                mqtt_start = 1;
            }
    }
}
//============================================================================


// WIFI����״̬�ı䣺���� = wifiStatus
//============================================================================
void wifiConnectCb(uint8_t status)
{
	// �ɹ���ȡ��IP��ַ
	//---------------------------------------------------------------------
    if(status == STATION_GOT_IP)
    {
    	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

    	// �ڹٷ����̵Ļ����ϣ�����2�����÷�����
    	//---------------------------------------------------------------
    	sntp_setservername(0, "us.pool.ntp.org");	// ������_0��������
    	sntp_setservername(1, "ntp.sjtu.edu.cn");	// ������_1��������

    	ipaddr_aton("210.72.145.44", addr);	// ���ʮ���� => 32λ������
    	sntp_setserver(2, addr);					// ������_2��IP��ַ��
    	os_free(addr);								// �ͷ�addr

    	sntp_init();	// SNTP��ʼ��


        // ����SNTP��ʱ��[sntp_timer]
        //-----------------------------------------------------------
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);		// 1s��ʱ
    }

    // IP��ַ��ȡʧ��
	//----------------------------------------------------------------
    else
    {
          MQTT_Disconnect(&mqttClient);	// WIFI���ӳ���TCP�Ͽ�����
    }
}
//============================================================================


// MQTT�ѳɹ����ӣ�ESP8266���͡�CONNECT���������յ���CONNACK��
//============================================================================
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡmqttClientָ��

    INFO("MQTT: Connected\r\n");

    // ������2����������� / ����3������Qos��
    //-----------------------------------------------------------------
//	MQTT_Subscribe(client, "SW_LED", 0);	// ��������"SW_LED"��QoS=0
//	MQTT_Subscribe(client, "SW_LED", 1);
//	MQTT_Subscribe(client, "SW_LED", 2);

	// ������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain��
	//-----------------------------------------------------------------------------------------------------------------------------------------
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// ������"SW_LED"����"ESP8266_Online"��Qos=0��retain=0
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 1, 0);
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 2, 0);
}
//============================================================================

// MQTT�ɹ��Ͽ�����
//============================================================================
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}
//============================================================================

// MQTT�ɹ�������Ϣ
//============================================================================
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================

// ������MQTT��[PUBLISH]���ݡ�����		������1������ / ����2�����ⳤ�� / ����3����Ч�غ� / ����4����Ч�غɳ��ȡ�
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// ���롾���⡿�ռ�
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// ���롾��Ч�غɡ��ռ�


    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡMQTT_Clientָ��


    os_memcpy(topicBuf, topic, topic_len);	// ��������
    topicBuf[topic_len] = 0;				// �����'\0'

    os_memcpy(dataBuf, data, data_len);		// ������Ч�غ�
    dataBuf[data_len] = 0;					// �����'\0'

    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// ���ڴ�ӡ�����⡿����Ч�غɡ�


// ����С�¡����
//########################################################################################
	// ���ݽ��յ���������/��Ч�غɣ�����LED����/��
	//-----------------------------------------------------------------------------------
	if( os_strcmp(topicBuf,"SW_LED") == 0 )			// ���� == "SW_LED"
	{
		if( os_strcmp(dataBuf,"LED_ON") == 0 )		// ��Ч�غ� == "LED_ON"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED��
		}

		else if( os_strcmp(dataBuf,"LED_OFF") == 0 )	// ��Ч�غ� == "LED_OFF"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);			// LED��
		}
	}
//########################################################################################


    os_free(topicBuf);	// �ͷš����⡿�ռ�
    os_free(dataBuf);	// �ͷš���Ч�غɡ��ռ�
}
//===============================================================================================================

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
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
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

// user_init��entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);	// ���ڲ�������Ϊ115200
    os_delay_us(60000);

//	// OLED��ʼ��
//	//����������������������������������������������
	OLED_Init();								// |
	OLED_ShowString(0,0,"ESP8266 = STA");		// |
	OLED_ShowString(0,4,"WIFI Connecting ");	// |
	OLED_ShowString(0,6,"................");	// |
	//////////
	OLED_ShowString(0,0,"                ");	// Internet Time
	OLED_ShowString(0,2,"Clock =         ");	// Clock��ʱ��
	OLED_ShowString(0,4,"Temp  =         ");	// Temperature���¶�
	OLED_ShowString(0,6,"Humid =         ");	// Humidity��ʪ��
//	//����������������������������������������������

//����С�¡����
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4�����	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// LED��ʼ��	#
//###########################################################################


    CFG_Load();	// ����/����ϵͳ������WIFI������MQTT������


    // �������Ӳ�����ֵ�������������mqtt_test_jx.mqtt.iot.gz.baidubce.com�����������Ӷ˿ڡ�1883������ȫ���͡�0��NO_TLS��
	//-------------------------------------------------------------------------------------------------------------------
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);

	// MQTT���Ӳ�����ֵ���ͻ��˱�ʶ����..����MQTT�û�����..����MQTT��Կ��..������������ʱ����120s��������Ự��1��clean_session��
	//----------------------------------------------------------------------------------------------------------------------------
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	// ������������(����ƶ�û�ж�Ӧ���������⣬��MQTT���ӻᱻ�ܾ�)
	//--------------------------------------------------------------
//	MQTT_InitLWT(&mqttClient, "Will", "ESP8266_offline", 0, 0);


	// ����MQTT��غ���
	//--------------------------------------------------------------------------------------------------
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// ���á�MQTT�ɹ����ӡ���������һ�ֵ��÷�ʽ
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// ���á�MQTT�ɹ��Ͽ�����������һ�ֵ��÷�ʽ
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// ���á�MQTT�ɹ���������������һ�ֵ��÷�ʽ
	MQTT_OnData(&mqttClient, mqttDataCb);					// ���á�����MQTT���ݡ���������һ�ֵ��÷�ʽ


	// ����WIFI��SSID[..]��PASSWORD[..]��WIFI���ӳɹ�����[wifiConnectCb]
	//--------------------------------------------------------------------------
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);


	INFO("\r\nSystem started ...\r\n");
}
//===================================================================================================================
