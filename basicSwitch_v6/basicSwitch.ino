/*****************************************************************************************
* FILENAME :        basicSwitch.c             DESIGN REF: 00001FW
*
* DESCRIPTION :
*       File to support SONOFF basic 
*
* PUBLIC FUNCTIONS :
*       boolean processPublishRequests(void)
*
* NOTES :
*       This module supports the WIFI configuration and FW Update
*       based on the library:
*       https://github.com/tzapu/WiFiManager
*       ssid of config page: OPEN_ESP_CONFIG_AP2
*       ip address: 192.168.4.1
*       Also toggleing the button at start will startup the WIFI
*       configuration.
*       
*       The basicSwitch implements the MQTT sonoff basic switch
*       functionality to turn on/off the relay in the switch. Additional
*       the LED will be switched to indicate the status of the 
*       relay.
*       
*
*       Copyright A.N.Other Co. 2017.  All rights reserved.
* 
* AUTHOR :    Stephan Wink        START DATE :    01.10.2017
*
* REF NO  VERSION DATE    WHO     DETAIL
* 000       16.10         SWI     First working version      
* 002       18.10         SWI     Add cmd mqtt request subscription
* 003       19.10         SWI     Add goto WifiManager command     
* 006       31.10         SWI     migrated first running version from template V1
* 007       11.11         SWI     add generic command subscription
*****************************************************************************************/

/*****************************************************************************************
 * Include Interfaces
*****************************************************************************************/
#include <ESP8266WiFi.h>
#include "WiFiManager.h" // local modified version          
#include <PubSubClient.h>
#include <EEPROM.h>
#include "gensettings.h"

#include "prjsettings.h"

/*****************************************************************************************
 * Local constant defines
*****************************************************************************************/

/*****************************************************************************************
 * Local function like makros 
*****************************************************************************************/

/*****************************************************************************************
 * Local type definitions (enum, struct, union):
*****************************************************************************************/

/*****************************************************************************************
 * Global data definitions (unlimited visibility, to be avoided):
*****************************************************************************************/

/****************************************************************************************/
/* Local function prototypes (always use static to limit visibility): 
*****************************************************************************************/
static void TurnRelaisOff(void);
static void TurnRelaisOn(void);
static void setSimpleLight(void);

/*****************************************************************************************
 * Local data definitions:
 * (always use static: limit visibility, const: read only, volatile: non-optimizable) 
*****************************************************************************************/
static boolean              simpleLightState_bolst = false;
static boolean              publishState_bolst = true;

/*****************************************************************************************
* Global functions (unlimited visibility): 
*****************************************************************************************/

/**---------------------------------------------------------------------------------------
 * @brief     This function processes the publish requests and is called cyclic.   
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    This function returns 'true' if the function processing was successful.
 *             In all other cases it returns 'false'.
*//*-----------------------------------------------------------------------------------*/
boolean basicSwitch_ProcessPublishRequests(void)
{
  String tPayload;
  boolean ret = false;

  if(true == publishState_bolst)
  {
    Serial.print("[mqtt] publish requested state: ");
    Serial.print(MQTT_PUB_LIGHT_STATE);
    Serial.print("  :  ");
    if(true == simpleLightState_bolst)
    {
      ret = client_sts.publish(build_topic(MQTT_PUB_LIGHT_STATE), MQTT_PAYLOAD_CMD_ON, true);
      Serial.println(MQTT_PAYLOAD_CMD_ON);
    }
    else
    {
      ret = client_sts.publish(build_topic(MQTT_PUB_LIGHT_STATE), MQTT_PAYLOAD_CMD_OFF, true); 
      Serial.println(MQTT_PAYLOAD_CMD_OFF); 
    } 
    if(ret)
    {
      publishState_bolst = false;     
    }
  }
 
  return ret;  
}

/**---------------------------------------------------------------------------------------
 * @brief     This callback function processes incoming MQTT messages and is called by   
 *              the PubSub client
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     p_topic     topic which was received
 * @param     p_payload   payload of the received MQTT message
 * @param     p_length    payload length
 * @return    This function returns 'true' if the function processing was successful.
 *             In all other cases it returns 'false'.
*//*-----------------------------------------------------------------------------------*/
void basicSwitch_CallbackMqtt(char* p_topic, String p_payload) 
{

  if (String(build_topic(MQTT_SUB_TOGGLE)).equals(p_topic)) 
  {
    basicSwitch_ToggleSimpleLight();
  }
  // execute command to switch on/off the light
  else if (String(build_topic(MQTT_SUB_BUTTON)).equals(p_topic)) 
  {
    // test if the payload is equal to "ON" or "OFF"
    if(0 == p_payload.indexOf(String(MQTT_PAYLOAD_CMD_ON))) 
    {
      simpleLightState_bolst = true;
      setSimpleLight();  
    }
    else if(0 == p_payload.indexOf(String(MQTT_PAYLOAD_CMD_OFF)))
    {
      simpleLightState_bolst = false;
      setSimpleLight();
    }
    else
    {
      Serial.print("[mqtt] unexpected command: "); 
      Serial.println(p_payload);
    }   
  }  
}

/**---------------------------------------------------------------------------------------
 * @brief     This initializes the used input and output pins
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    void
*//*-----------------------------------------------------------------------------------*/
void basicSwitch_InitializePins(void)
{
  pinMode(SIMPLE_LIGHT_PIN, OUTPUT);
  setSimpleLight();
  pinMode(LED_PIN, OUTPUT);
}

/**---------------------------------------------------------------------------------------
 * @brief     This function toggles the relais
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    void
*//*-----------------------------------------------------------------------------------*/
void basicSwitch_ToggleSimpleLight(void)
{
  if(true == simpleLightState_bolst)
  {   
    TurnRelaisOff();  
  }
  else
  {   
    TurnRelaisOn();
  }
}

/**---------------------------------------------------------------------------------------
 * @brief     This function handles the connection to the MQTT broker. If connection can't
 *              be established after several attempts the WifiManager is called. If 
 *              connection is successfull, all needed subscriptions are done.
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void basicSwitch_Reconnect() 
{ 
  // ... and resubscribe
  client_sts.subscribe(build_topic(MQTT_SUB_TOGGLE));  // toggle sonoff button
  client_sts.loop();
  Serial.print("[mqtt] subscribed 1: ");
  Serial.println(MQTT_SUB_TOGGLE);
  client_sts.subscribe(build_topic(MQTT_SUB_BUTTON));  // change button state with payload
  client_sts.loop();
  Serial.print("[mqtt] subscribed 2: ");
  Serial.println(MQTT_SUB_BUTTON);
  client_sts.loop();
}

/****************************************************************************************/
/* Local functions (always use static to limit visibility): */
/****************************************************************************************/

/**---------------------------------------------------------------------------------------
 * @brief     This function turns the relais off
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    void
*//*-----------------------------------------------------------------------------------*/
static void TurnRelaisOff(void)
{
  simpleLightState_bolst = false;
  digitalWrite(SIMPLE_LIGHT_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("relais state: OFF");
  Serial.println("request publish");
  publishState_bolst = true;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function turns the relais on
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    void
*//*-----------------------------------------------------------------------------------*/
static void TurnRelaisOn(void)
{
  simpleLightState_bolst = true;
  digitalWrite(SIMPLE_LIGHT_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Button state: ON");
  Serial.println("request publish");
  publishState_bolst = true;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function sets the relay based on the state of the global variable
 *              simpleLightState_bolst
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void setSimpleLight(void)
{
  if(true == simpleLightState_bolst)
  {
    TurnRelaisOn(); 
  }
  else
  {
    TurnRelaisOff(); 
  }
}


/****************************************************************************************/
