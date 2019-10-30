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


//////////////////////////////////////////////////////////////////////////////////////////////////////
//														//	{										//
// 工程：	JSON_API									//		"Shanghai": {						//
//														//			"temp": "30℃",					//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0		//			"humid": "30%RH"				//
//														//		},									//
// 功能：	①：创建JSON树【使用JSON_API】				//		"Shenzhen": {						//
//														//			"temp": "35℃",					//
//			②：解析JSON树【使用JSON_API】				//			"humid": 50						//
//														//		},									//
//	版本：	V1.0										//		"result": "Shenzhen is too hot!"	//
//														//	}										//
//////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口

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
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro

#include "user_json.h"			// JSON
//==================================================================================


/* 【JSON_Tree】
//===========================================
	{
    	"Shanghai":
    	{
        	"temp": "30℃",
        	"humid": "30%RH"
    	},

    	"Shenzhen":
    	{
        	"temp": "35℃",
        	"humid": 50
		},

    	"result": "Shenzhen is too hot!"
	}

//===========================================
*/


// 宏定义
//==================================================================================
#define		ProjectName			"JSON_API"			// 工程名宏定义
//==================================================================================


// 全局变量
//==================================================================================
char A_JSON_Tree[256] = {0};	// 存放JSON树
//==================================================================================

// 函数声明
//============================================================================================================
int ICACHE_FLASH_ATTR JSON_Tree_Parse_Cb(struct jsontree_context *js_ctx, struct jsonparse_state *parser);
//============================================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// 创建JSON树的回调函数：JSON树赋值
//【jsontree_path_name(...)：		获取JSON树对应深度的【"键"】的指针】
//【jsontree_write_string(...)：	赋值JSON树对应深度的【"键"】(以字符串形式赋值)】
//【jsontree_write_atom(...)：		赋值JSON树对应深度的【"键"】(以数值的形式赋值)】
//=======================================================================================================================
LOCAL int ICACHE_FLASH_ATTR JSON_AssignValue_Cb(struct jsontree_context *js_ctx)	// ①：定义生成JSON树的函数
{
	const char * P_Key_current = jsontree_path_name(js_ctx,js_ctx->depth-1);	// 获取指向当前【"键"】的指针

	if( os_strncmp(P_Key_current, "result", os_strlen("result")) == 0 )			// 判断当前【"键"】 ?= "result"
	{
		jsontree_write_string(js_ctx,"Shenzhen is too hot!");	// 将【"值"="Shenzhen is too hot!"】写入对应位置
	}

	else	// 当前【"键"】 != "result"，判断上一层指针
	{
		const char * P_Key_upper = jsontree_path_name(js_ctx,js_ctx->depth-2);		// 获取指向上一层【"键"】的指针


		if(os_strncmp(P_Key_upper, "Shanghai", os_strlen("Shanghai")) == 0)			// 判断上一层【"键"】 ?= "Shanghai"
		{
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)			// 判断当前【"键"】 ?= "temp"
			{
				jsontree_write_string(js_ctx,"30℃");	// 将【"值"="30℃"】写入对应位置
			}


			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)			// 判断当前【"键"】 ?= "humid"
			{
				jsontree_write_string(js_ctx,"30%RH");	// 将【"值"="30%RH"】写入对应位置
			}

		}

		else if( os_strncmp(P_Key_upper, "Shenzhen", os_strlen("Shenzhen")) == 0 )	// 判断上一层【"键"】 ?= "Shenzhen"
		{
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)			// 判断当前【"键"】 ?= "temp"
			{
				jsontree_write_string(js_ctx,"35℃");	// 将【"值"="35℃"】写入对应位置
			}


			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)			// 判断当前【"键"】 ?= "humid"
			{
				jsontree_write_atom(js_ctx,"50");		// 将【"值" = 50】写入对应位置
			}
		}
	}

    return 0;
}
//=======================================================================================================================



// JSON树相关设置
//============================================================================================================================

// 设置【创建JSON树】、【解析JSON树】的回调函数
//【如果觉得放在一个赋"值"函数中麻烦，可以定义多个JSON复制函数】
struct jsontree_callback JSON_Tree_Set = JSONTREE_CALLBACK(JSON_AssignValue_Cb, JSON_Tree_Parse_Cb);	// 结构体定义


//【对象名：V_Key_1		键值对："temp":{ &JSON_Tree_Set }, "humid":{ &JSON_Tree_Set }】
//【对象名：V_Key_2		键值对："temp":{ &JSON_Tree_Set }, "humid":{ &JSON_Tree_Set }】
//--------------------------------------------------------------------------------------------------------------------------
JSONTREE_OBJECT(V_Key_1,JSONTREE_PAIR("temp", &JSON_Tree_Set),JSONTREE_PAIR("humid", &JSON_Tree_Set));	// 设置【"键":"值"】
JSONTREE_OBJECT(V_Key_2,JSONTREE_PAIR("temp", &JSON_Tree_Set),JSONTREE_PAIR("humid", &JSON_Tree_Set));	// 设置【"键":"值"】


//【对象名：V_JSON		键值对："Shanghai":{ &V_Key_1 }, "Shenzhen":{ &V_Key_1 }, "result":{ &JSON_Tree_Set }】
//----------------------------------------------------------------------------------------------------------------
JSONTREE_OBJECT(	V_JSON, 	JSONTREE_PAIR("Shanghai", &V_Key_1),	JSONTREE_PAIR("Shenzhen", &V_Key_2),	\
								JSONTREE_PAIR("result", &JSON_Tree_Set)	);		// 设置【"键":"值"】


//---------------------------------------------------------------------------
// 注：JSONTREE_OBJECT( 参数1, 参数2, 参数3, ... )

//【参数1：生成的JSON树对象的名称】、【参数2：键值对】、【参数3：键值对】...
//---------------------------------------------------------------------------
//【对象名：Object_JOSN	  键值对："K_JOSN" : { &V_JSON } 】

//【注：第一个的JSON对象名、"键"都不显示。只显示：{ &V_JSON } 】
//----------------------------------------------------------------------------------
JSONTREE_OBJECT(Object_JOSN,JSONTREE_PAIR("K_JOSN", &V_JSON));		// 生成JOSN对象

//============================================================================================================================


// 创建JSON树
//================================================================================
void ICACHE_FLASH_ATTR Setup_JSON_Tree_JX(void)
{
	//【创建JSON树】
	//【参数1：首JSON对象的指针 / 参数2：首JSON对象的键 / 参数3：JSON树缓存指针】
	//---------------------------------------------------------------------------
	json_ws_send((struct jsontree_value *)&Object_JOSN, "K_JOSN", A_JSON_Tree);


	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");

	os_printf("%s",A_JSON_Tree);	// 串口打印JSON树

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");
}
//================================================================================


// 解析JSON树
//=====================================================================================================
void ICACHE_FLASH_ATTR Parse_JSON_Tree_JX(void)
{
	struct jsontree_context js;		// JSON树环境结构体

	// 和要解析的JSON树建立联系
	//【参数1：JSON树环境结构体指针		参数2：JSON树的第二个对象名指针		参数3：json_putchar函数】
	//-----------------------------------------------------------------------------------------------
	jsontree_setup(&js, (struct jsontree_value *)&V_JSON, json_putchar);

	// 解析JSON树
	//【参数1：JSON树环境结构体指针		参数2：要解析的JSON树的指针】
	json_parse(&js, A_JSON_Tree);	// 执行这条语句，将会调用对应的JSON树解析回调函数
}
//=====================================================================================================


// 解析JSON树的回调函数
//========================================================================================================================================
int ICACHE_FLASH_ATTR JSON_Tree_Parse_Cb(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

    int type;						// 字符类型

    char buffer[64] = {0};			// 缓存【值】的数组

    type = jsonparse_next(parser);	// 【{】：JSON对象的框架首字符
    os_printf("%c\n", type);

    type = jsonparse_next(parser);	// 【键N】：第一个顶级键【Shanghai】


    if(type != 0)	// 判断是否有字符串
    {
        if (type == JSON_TYPE_PAIR_NAME) 	// 判断是否是【键N】
        {
            if (jsonparse_strcmp_value(parser, "Shanghai") == 0)	// 【Shanghai】
            {
            	os_printf("\t Shanghai{2} \n");

            	type = jsonparse_next(parser);		// 【:】
            	type = jsonparse_next(parser);		// 【{】

            	type = jsonparse_next(parser);  	// 【键N】
                if (jsonparse_strcmp_value(parser, "temp") == 0)	// 【temp】
                {
                	type = jsonparse_next(parser);		// 【:】
                	type = jsonparse_next(parser);  	// 【"】

                	if (type == JSON_TYPE_STRING)		// 判断是否是【"】
					{
						jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
						os_printf("\t\t temp: %s\n", buffer);		// 【30℃】
					}

                	type = jsonparse_next(parser);  	// 【,】

                	type = jsonparse_next(parser);  	// 【键N】
                	if (jsonparse_strcmp_value(parser, "humid") == 0)	// 【humid】
                	{
                		type = jsonparse_next(parser);  	// 【:】

                		type = jsonparse_next(parser);  	// 【"】
						if (type == JSON_TYPE_STRING)		// 判断是否是【"】
						{
							jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
							os_printf("\t\t humid: %s\n", buffer);	// 【30%RH】


							type = jsonparse_next(parser);  	// 【}】
							type = jsonparse_next(parser);  	// 【,】


							type = jsonparse_next(parser);  	//【键N】：第二个顶级键【Shenzhen】
							if (jsonparse_strcmp_value(parser, "Shenzhen") == 0)	// 【Shenzhen】
							{
								os_printf("\t Shenzhen{2} \n");

								jsonparse_next(parser);		// 【:】
								jsonparse_next(parser);		// 【{】

								jsonparse_next(parser);  	// 【键N】
								if (jsonparse_strcmp_value(parser, "temp") == 0)	// 【temp】
								{
									type = jsonparse_next(parser);		// 【:】

									type = jsonparse_next(parser);  	// 【"】
									if (type == JSON_TYPE_STRING)		// 判断是否是【"】
									{
										jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
										os_printf("\t\t temp: %s\n", buffer);	// 【35℃】

										type = jsonparse_next(parser);  	// 【,】

										type = jsonparse_next(parser);  	// 【键N】
										if (jsonparse_strcmp_value(parser, "humid") == 0)	// 【humid】
										{
											type = jsonparse_next(parser);  	// 【:】

											type = jsonparse_next(parser);  	// 【0】("值" = 数值)
											if (type == JSON_TYPE_NUMBER)		// 判断是否是【0】(数值 == ASSIC码形式)
											{
												jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
												os_printf("\t\t humid: %s\n", buffer);	// 【50%RH】

												type = jsonparse_next(parser);  	// 【}】
												type = jsonparse_next(parser);  	// 【,】

												type = jsonparse_next(parser);  	//【键N】：第三个顶级键【result】
												if (jsonparse_strcmp_value(parser, "result") == 0)		// 【result】
												{
													type = jsonparse_next(parser);  // 【:】

													type = jsonparse_next(parser);  // 【"】
													if (type == JSON_TYPE_STRING)	// 判断是否是【"】
													{
														jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
														os_printf("\t result: %s\n", buffer);	//【Shenzhen is too hot!】

														type = jsonparse_next(parser);	// 【}】：JSON对象的框架尾字符
														os_printf("%c\n", type);
													}
												}
											}
										}
									}
								}
							}
						}
                	}
                }
            }
        }
    }

    os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

    return 0;
}
//========================================================================================================================================



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

	Setup_JSON_Tree_JX();		// 创建JSON树

	Parse_JSON_Tree_JX();		// 解析JSON树
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
