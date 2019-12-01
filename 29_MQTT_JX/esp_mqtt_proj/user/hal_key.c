/**
************************************************************
* @file         hal_key.c
* @brief        鎸夐敭椹卞姩
* 
* 鎸夐敭妯″潡閲囩敤瀹氭椂鍣�+GPIO鐘舵�佽鍙栨満鍒讹紝GPIO鐨勯厤缃鏍规嵁ESP8266鐨勭浉鍏虫墜鍐屾潵閰嶇疆
* 
* 鏈┍鍔ㄦ敮鎸� 0 ~ 12 涓狦PIO鎸夐敭鎵╁睍锛屾敮鎸佽法骞冲彴绉绘銆�
* @author       Gizwits
* @date         2016-09-05
* @version      V03010201
* @copyright    Gizwits
* 
* @note         鏈烘櫤浜�.鍙负鏅鸿兘纭欢鑰岀敓
*               Gizwits Smart Cloud  for Smart Products
*               閾炬帴|澧炲�紎寮�鏀緗涓珛|瀹夊叏|鑷湁|鑷敱|鐢熸��
*               www.gizwits.com
*
***********************************************************/
#include "hal_key.h"
#include "mem.h"

uint32 keyCountTime = 0; 
static uint8_t keyTotolNum = 0;

/**
* @brief Read the GPIO state
* @param [in] keys 鎸夐敭鍔熻兘鍏ㄥ眬缁撴瀯浣撴寚閽�
* @return uint16_t鍨嬬殑GPIO鐘舵�佸��
*/
static ICACHE_FLASH_ATTR uint16_t keyValueRead(keys_typedef_t * keys)
{
    uint8_t i = 0; 
    uint16_t read_key = 0;

    //GPIO Cyclic scan
    for(i = 0; i < keys->keyTotolNum; i++)
    {
        if(!GPIO_INPUT_GET(keys->singleKey[i]->gpio_id))
        {
            G_SET_BIT(read_key, keys->singleKey[i]->gpio_number);
        }
    }
    
    return read_key;
}

/**
* @brief Read the KEY value
* @param [in] keys 鎸夐敭鍔熻兘鍏ㄥ眬缁撴瀯浣撴寚閽�
* @return uint16_t鍨嬬殑鎸夐敭鐘舵�佸��
*/
static uint16_t ICACHE_FLASH_ATTR keyStateRead(keys_typedef_t * keys)
{
    static uint8_t Key_Check = 0;
    static uint8_t Key_State = 0;
    static uint16_t Key_LongCheck = 0;
    uint16_t Key_press = 0; 
    uint16_t Key_return = 0;
    static uint16_t Key_Prev = 0;
    
    //绱姞鎸夐敭鏃堕棿
    keyCountTime++;
        
    //鎸夐敭娑堟姈30MS
    if(keyCountTime >= (DEBOUNCE_TIME / keys->key_timer_ms)) 
    {
        keyCountTime = 0; 
        Key_Check = 1;
    }
    
    if(Key_Check == 1)
    {
        Key_Check = 0;
        
        //鑾峰彇褰撳墠鎸夐敭瑙﹀彂鍊�
        Key_press = keyValueRead(keys); 
        
        switch (Key_State)
        {
            //"棣栨鎹曟崏鎸夐敭"鐘舵��
            case 0:
                if(Key_press != 0)
                {
                    Key_Prev = Key_press;
                    Key_State = 1;
                }
    
                break;
                
                //"鎹曟崏鍒版湁鏁堟寜閿�"鐘舵��
            case 1:
                if(Key_press == Key_Prev)
                {
                    Key_State = 2;
                    Key_return= Key_Prev | KEY_DOWN;
                }
                else
                {
                    //鎸夐敭鎶捣,鏄姈鍔�,涓嶅搷搴旀寜閿�
                    Key_State = 0;
                }
                break;
                
                //"鎹曟崏闀挎寜閿�"鐘舵��
            case 2:
    
                if(Key_press != Key_Prev)
                {
                    Key_State = 0;
                    Key_LongCheck = 0;
                    Key_return = Key_Prev | KEY_UP;
                    return Key_return;
                }
    
                if(Key_press == Key_Prev)
                {
                    Key_LongCheck++;
                    if(Key_LongCheck >= (PRESS_LONG_TIME / DEBOUNCE_TIME))    //闀挎寜3S (娑堟姈30MS * 100)
                    {
                        Key_LongCheck = 0;
                        Key_State = 3;
                        Key_return= Key_press |  KEY_LONG;
                        return Key_return;
                    }
                }
                break;
                
                //"杩樺師鍒濆"鐘舵��    
            case 3:
                if(Key_press != Key_Prev)
                {
                    Key_State = 0;
                }
                break;
        }
    }

    return  NO_KEY;
}

/**
* @brief 鎸夐敭鍥炶皟鍑芥暟

* 鍦ㄨ鍑芥暟鍐呭畬鎴愭寜閿姸鎬佺洃娴嬪悗璋冪敤瀵瑰簲鐨勫洖璋冨嚱鏁�
* @param [in] keys 鎸夐敭鍔熻兘鍏ㄥ眬缁撴瀯浣撴寚閽�
* @return none
*/
void ICACHE_FLASH_ATTR gokitKeyHandle(keys_typedef_t * keys)
{
    uint8_t i = 0;
    uint16_t key_value = 0;

    key_value = keyStateRead(keys); 
    
    if(!key_value) return;
    
    //Check short press button
    if(key_value & KEY_UP)
    {
        //Valid key is detected
        for(i = 0; i < keys->keyTotolNum; i++)
        {
            if(G_IS_BIT_SET(key_value, keys->singleKey[i]->gpio_number)) 
            {
                //key callback function of short press
                if(keys->singleKey[i]->short_press) 
                {
                    keys->singleKey[i]->short_press(); 
                    
                    os_printf("[zs] callback key: [%d][%d] \r\n", keys->singleKey[i]->gpio_id, keys->singleKey[i]->gpio_number); 
                }
            }
        }
    }

    //Check short long button
    if(key_value & KEY_LONG)
    {
        //Valid key is detected
        for(i = 0; i < keys->keyTotolNum; i++)
        {
            if(G_IS_BIT_SET(key_value, keys->singleKey[i]->gpio_number))
            {
                //key callback function of long press
                if(keys->singleKey[i]->long_press) 
                {
                    keys->singleKey[i]->long_press(); 
                    
                    os_printf("[zs] callback long key: [%d][%d] \r\n", keys->singleKey[i]->gpio_id, keys->singleKey[i]->gpio_number); 
                }
            }
        }
    }
}

/**
* @brief 鍗曟寜閿垵濮嬪寲

* 鍦ㄨ鍑芥暟鍐呭畬鎴愬崟涓寜閿殑鍒濆鍖栵紝杩欓噷闇�瑕佺粨鍚圗SP8266 GPIO瀵勫瓨鍣ㄨ鏄庢枃妗ｆ潵璁剧疆鍙傛暟
* @param [in] gpio_id ESP8266 GPIO 缂栧彿
* @param [in] gpio_name ESP8266 GPIO 鍚嶇О
* @param [in] gpio_func ESP8266 GPIO 鍔熻兘
* @param [in] long_press 闀挎寜鐘舵�佺殑鍥炶皟鍑芥暟鍦板潃
* @param [in] short_press 鐭寜鐘舵�佺殑鍥炶皟鍑芥暟鍦板潃
* @return 鍗曟寜閿粨鏋勪綋鎸囬拡
*/
key_typedef_t * ICACHE_FLASH_ATTR keyInitOne(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, gokit_key_function long_press, gokit_key_function short_press)
{
    static int8_t key_total = -1;

    key_typedef_t * singleKey = (key_typedef_t *)os_zalloc(sizeof(key_typedef_t));

    singleKey->gpio_number = ++key_total;
    
    //Platform-defined GPIO
    singleKey->gpio_id = gpio_id;
    singleKey->gpio_name = gpio_name;
    singleKey->gpio_func = gpio_func;
    
    //Button trigger callback type
    singleKey->long_press = long_press;
    singleKey->short_press = short_press;
    
    keyTotolNum++;    

    return singleKey;
}

/**
* @brief 鎸夐敭椹卞姩鍒濆鍖�

* 鍦ㄨ鍑芥暟鍐呭畬鎴愭墍鏈夌殑鎸夐敭GPIO鍒濆鍖栵紝骞跺紑鍚竴涓畾鏃跺櫒寮�濮嬫寜閿姸鎬佺洃娴�
* @param [in] keys 鎸夐敭鍔熻兘鍏ㄥ眬缁撴瀯浣撴寚閽�
* @return none
*/
void ICACHE_FLASH_ATTR keyParaInit(keys_typedef_t * keys)
{
    uint8 tem_i = 0; 
    
    if(NULL == keys)
    {
        return ;
    }
    
    //init key timer 
    keys->key_timer_ms = KEY_TIMER_MS; 
    os_timer_disarm(&keys->key_timer); 
    os_timer_setfn(&keys->key_timer, (os_timer_func_t *)gokitKeyHandle, keys); 
    
    keys->keyTotolNum = keyTotolNum;

    //Limit on the number keys (Allowable number: 0~12)
    if(KEY_MAX_NUMBER < keys->keyTotolNum) 
    {
        keys->keyTotolNum = KEY_MAX_NUMBER; 
    }
    
    //GPIO configured as a high level input mode
    for(tem_i = 0; tem_i < keys->keyTotolNum; tem_i++) 
    {
        PIN_FUNC_SELECT(keys->singleKey[tem_i]->gpio_name, keys->singleKey[tem_i]->gpio_func); 
        GPIO_OUTPUT_SET(GPIO_ID_PIN(keys->singleKey[tem_i]->gpio_id), 1); 
        PIN_PULLUP_EN(keys->singleKey[tem_i]->gpio_name); 
        GPIO_DIS_OUTPUT(GPIO_ID_PIN(keys->singleKey[tem_i]->gpio_id)); 
        
        os_printf("gpio_name %d \r\n", keys->singleKey[tem_i]->gpio_id); 
    }
    
    //key timer start
    os_timer_arm(&keys->key_timer, keys->key_timer_ms, 1); 
}

/**
* @brief 鎸夐敭椹卞姩娴嬭瘯

* 璇ュ嚱鏁版ā鎷熶簡澶栭儴瀵规寜閿ā鍧楃殑鍒濆鍖栬皟鐢�

* 娉細鐢ㄦ埛闇�瑕佸畾涔夌浉搴旂殑鎸夐敭鍥炶皟鍑芥暟(濡� key1LongPress ...)
* @param none
* @return none
*/
void ICACHE_FLASH_ATTR keyTest(void)
{
#ifdef KEY_TEST
    //鎸夐敭GPIO鍙傛暟瀹忓畾涔�
    #define GPIO_KEY_NUM                            2                           ///< 瀹氫箟鎸夐敭鎴愬憳鎬绘暟
    #define KEY_0_IO_MUX                            PERIPHS_IO_MUX_GPIO0_U      ///< ESP8266 GPIO 鍔熻兘
    #define KEY_0_IO_NUM                            0                           ///< ESP8266 GPIO 缂栧彿
    #define KEY_0_IO_FUNC                           FUNC_GPIO0                  ///< ESP8266 GPIO 鍚嶇О
    #define KEY_1_IO_MUX                            PERIPHS_IO_MUX_MTMS_U       ///< ESP8266 GPIO 鍔熻兘
    #define KEY_1_IO_NUM                            14                          ///< ESP8266 GPIO 缂栧彿
    #define KEY_1_IO_FUNC                           FUNC_GPIO14                 ///< ESP8266 GPIO 鍚嶇О
    LOCAL key_typedef_t * singleKey[GPIO_KEY_NUM];                              ///< 瀹氫箟鍗曚釜鎸夐敭鎴愬憳鏁扮粍鎸囬拡
    LOCAL keys_typedef_t keys;                                                  ///< 瀹氫箟鎬荤殑鎸夐敭妯″潡缁撴瀯浣撴寚閽�    
    
    //姣忓垵濮嬪寲涓�涓寜閿皟鐢ㄤ竴娆eyInitOne ,singleKey娆″簭鍔犱竴
    singleKey[0] = keyInitOne(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
                                key1LongPress, key1ShortPress);
                                
    singleKey[1] = keyInitOne(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
                                key2LongPress, key2ShortPress);
                                
    keys.key_timer_ms = KEY_TIMER_MS; //璁剧疆鎸夐敭瀹氭椂鍣ㄥ懆鏈� 寤鸿10ms
    keys.singleKey = singleKey; //瀹屾垚鎸夐敭鎴愬憳璧嬪��
    keyParaInit(&keys); //鎸夐敭椹卞姩鍒濆鍖�
#endif
}
