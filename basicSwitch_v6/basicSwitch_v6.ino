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
*
* REF NO  VERSION DATE    WHO     DETAIL
* 000       16.10         SWI     First working version      
* 002       18.10         SWI     Add cmd mqtt request subscription
* 003       19.10         SWI     Add goto WifiManager command     
* 006       31.10         SWI     migrated first running version from template V1
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

/*****************************************************************************************
 * Local data definitions:
 * (always use static: limit visibility, const: read only, volatile: non-optimizable) 
*****************************************************************************************/
static char buffer_stca[60];

// buffer used to send/receive data with MQTT, can not be done with the 
// buffer_stca, as both are needed simultaniously 
static WiFiClient            wifiClient_sts;
static PubSubClient          client_sts(wifiClient_sts);
static mqttData_t            mqttData_sts;

static WiFiManager           wifiManager_sts;
// prepare wifimanager variables
static WiFiManagerParameter  wifiManagerParamMqttServerId_sts("mq_ip", "mqtt server ip", "", 15);
static WiFiManagerParameter  wifiManagerParamMqttServerPort_sts("mq_port", "mqtt server port", "1883", 5);
static WiFiManagerParameter  wifiManagerParamMqttCapability_sts("cap", "Capability Bit0 = n/a, Bit1 = n/a, Bit2 = n/a", "", 2);
static WiFiManagerParameter  wifiManagerParamMqttClientShort_sts("sid", "mqtt short id", "devXX", 6);
static WiFiManagerParameter  wifiManagerParamMqttServerLogin_sts("login", "mqtt login", "", 15);
static WiFiManagerParameter  wifiManagerParamMqttServerPw_sts("pw", "mqtt pw", "", 15);

static uint32_t             timerRepubAvoid_u32st = 0;
static uint32_t             timerButtonDown_u32st  = 0;
static uint8_t              counterButton_u8st = 0;
static uint32_t             timerLastPub_u32st = 0;
static boolean              publishInfo_bolst = false;
static boolean              startWifiConfig_bolst = false;

//static boolean              simpleLightState_bolst = false;
//static boolean              publishState_bolst = true;

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
boolean processPublishRequests(void)
{
  String tPayload;
  boolean ret = false;

  if(true == publishInfo_bolst)
  {
    Serial.print("[mqtt] publish requested info: ");
    Serial.print(FW_IDENTIFIER);
    Serial.print(FW_VERSION);
    Serial.println(FW_DESCRIPTION);
    ret = client_sts.publish(build_topic(MQTT_PUB_FW_IDENT), FW_IDENTIFIER, true);
    ret &= client_sts.publish(build_topic(MQTT_PUB_FW_VERSION), FW_VERSION, true);
    ret &= client_sts.publish(build_topic(MQTT_PUB_FW_DESC), FW_DESCRIPTION, true);
    if(ret)
    {
      publishInfo_bolst = false;     
    }
  }
  else
  {
    ret = basicSwitch_ProcessPublishRequests();
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
void callback(char* p_topic, byte* p_payload, unsigned int p_length) 
{
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) 
  {
    payload.concat((char)p_payload[i]);
  }
  // print received topic and payload
  Serial.print("[mqtt] message received: ");
  Serial.print(p_topic);
  Serial.print("   ");
  Serial.println(payload);

  // execute generic support command
  if(String(build_topic(MQTT_SUB_COMMAND)).equals(p_topic)) 
  {
    // print firmware information
    if(0 == payload.indexOf(String(MQTT_PAYLOAD_CMD_INFO))) 
    {
      publishInfo_bolst = true;     
    }
    // goto wifimanager configuration 
    else if(0 == payload.indexOf(String(MQTT_PAYLOAD_CMD_SETUP))) 
    {
      startWifiConfig_bolst = true; 
    } 
    else
    {
      Serial.print("[mqtt] unexpected command: "); 
      Serial.println(payload);
    }
  }
  else
  {
    basicSwitch_CallbackMqtt(p_topic, payload); 
  }  
}

/**---------------------------------------------------------------------------------------
 * @brief     This initializes the used input and output pins
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    void
*//*-----------------------------------------------------------------------------------*/
void InitializePins(void)
{
  // attache interrupt code for button
  pinMode(BUTTON_INPUT_PIN, INPUT);
  digitalWrite(BUTTON_INPUT_PIN, HIGH); // pull up to avoid interrupts without sensor
  attachInterrupt(digitalPinToInterrupt(BUTTON_INPUT_PIN), updateBUTTONstate, CHANGE);
  basicSwitch_InitializePins();
}

/**---------------------------------------------------------------------------------------
 * @brief     This function handles the external button input and updates the value
 *              of the simpleLightState_bolst variable. If the value changed the corresponding
 *              set function is called. Debouncing is also handled by this function
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void updateBUTTONstate() 
{
  // toggle, write to pin, publish to server
  if(digitalRead(BUTTON_INPUT_PIN)==LOW)
  {
    if(millis() - timerButtonDown_u32st > BUTTON_DEBOUNCE)
    { // avoid bouncing
      // button down
      // toggle status of both lights
      basicSwitch_ToggleSimpleLight();

      if(millis() - timerButtonDown_u32st < BUTTON_TIMEOUT)
      {
        counterButton_u8st++;
      } else {
        counterButton_u8st=1;
      }
      Serial.print("[BUTTON] push nr ");
      Serial.println(counterButton_u8st);

    };
    //Serial.print(".");
    timerButtonDown_u32st = millis();
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
void reconnect() 
{
  // Loop until we're reconnected
  uint8_t tries=0;
  while (!client_sts.connected()) 
  {
    Serial.println("[mqtt] Attempting connection...");
    // Attempt to connect
    Serial.print("[mqtt] client id: ");
    Serial.println(mqttData_sts.dev_short);
    if (client_sts.connect(mqttData_sts.dev_short, mqttData_sts.login, mqttData_sts.pw)) {
      Serial.println("[mqtt] connected");

      // ... and resubscribe
      basicSwitch_Reconnect();

      Serial.println("[mqtt] subscribing finished");
      Serial.print("[mqtt] publish firmware info: ");
      Serial.print(FW_IDENTIFIER);
      Serial.print(FW_VERSION);
      Serial.println(FW_DESCRIPTION);
      client_sts.publish(build_topic(MQTT_PUB_FW_IDENT), FW_IDENTIFIER, true);
      client_sts.publish(build_topic(MQTT_PUB_FW_VERSION), FW_VERSION, true);
      client_sts.publish(build_topic(MQTT_PUB_FW_DESC), FW_DESCRIPTION, true);
      Serial.println("[mqtt] publishing finished");
    } 
    else 
    {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client_sts.state());
      Serial.println(", DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    tries++;
    if(tries>=5){
      Serial.println("Can't connect, starting AP");
      wifiManager_sts.startConfigPortal(CONFIG_SSID); // needs to be tested!
    }
  }
}

/**---------------------------------------------------------------------------------------
 * @brief     This callback function handles the wifimanager callback
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     myWiFiManager     pointer to the wifimanager
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void configModeCallback(WiFiManager *myWiFiManager) 
{
  wifiManager_sts.addParameter(&wifiManagerParamMqttServerId_sts);
  wifiManager_sts.addParameter(&wifiManagerParamMqttServerPort_sts);
  wifiManager_sts.addParameter(&wifiManagerParamMqttCapability_sts);
  wifiManager_sts.addParameter(&wifiManagerParamMqttClientShort_sts);
  wifiManager_sts.addParameter(&wifiManagerParamMqttServerLogin_sts);
  wifiManager_sts.addParameter(&wifiManagerParamMqttServerPw_sts);
  // prepare wifimanager variables
  wifiManager_sts.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,255), IPAddress(255,255,255,0));
  Serial.println("Entered config mode");
}

/**---------------------------------------------------------------------------------------
 * @brief     This callback function handles the wifimanager save callback
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void saveConfigCallback()
{ 
  sprintf(mqttData_sts.server_ip, "%s", wifiManagerParamMqttServerId_sts.getValue());
  sprintf(mqttData_sts.login, "%s", wifiManagerParamMqttServerLogin_sts.getValue());
  sprintf(mqttData_sts.pw, "%s", wifiManagerParamMqttServerPw_sts.getValue());
  sprintf(mqttData_sts.cap, "%s", wifiManagerParamMqttCapability_sts.getValue());
  sprintf(mqttData_sts.server_port, "%s", wifiManagerParamMqttServerPort_sts.getValue());
  sprintf(mqttData_sts.dev_short, "%s", wifiManagerParamMqttClientShort_sts.getValue());
  Serial.println(("=== Saving parameters: ==="));
  Serial.print(("mqtt ip: ")); Serial.println(mqttData_sts.server_ip);
  Serial.print(("mqtt port: ")); Serial.println(mqttData_sts.server_port);
  Serial.print(("mqtt user: ")); Serial.println(mqttData_sts.login);
  Serial.print(("capabilities: ")); Serial.println(mqttData_sts.cap);
  Serial.print(("mqtt pw: ")); Serial.println(mqttData_sts.pw);
  Serial.print(("mqtt dev short: ")); Serial.println(mqttData_sts.dev_short);
  Serial.println(("=== End of parameters ===")); 
  char* temp=(char*) &mqttData_sts;
  for(int i=0; i<sizeof(mqttData_sts); i++){
    EEPROM.write(i,*temp);
    //Serial.print(*temp);
    temp++;
  }
  EEPROM.commit();
  Serial.println("Configuration saved, restarting");
  delay(2000);  
  ESP.reset(); // eigentlich muss das gehen so, .. // we can't change from AP mode to client mode, thus: reboot
}


/**---------------------------------------------------------------------------------------
 * @brief     This function load the configuration from external eeprom
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     myWiFiManager     pointer to the wifimanager
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void loadConfig()
{
  // fill the mqtt element with all the data from eeprom
  char* temp=(char*) &mqttData_sts;
  for(int i=0; i<sizeof(mqttData_sts); i++){
    //Serial.print(i);
    *temp = EEPROM.read(i);
    //Serial.print(*temp);
    temp++;
  }
  Serial.println(("=== Loaded parameters: ==="));
  Serial.print(("mqtt ip: "));        Serial.println(mqttData_sts.server_ip);
  Serial.print(("mqtt port: "));      Serial.println(mqttData_sts.server_port);
  Serial.print(("mqtt user: "));      Serial.println(mqttData_sts.login);
  Serial.print(("mqtt pw: "));        Serial.println(mqttData_sts.pw);
  Serial.print(("mqtt dev short: ")); Serial.println(mqttData_sts.dev_short);
  Serial.print(("capabilities: "));   Serial.println(mqttData_sts.cap);

  // capabilities
  // capabilities
  
  Serial.println(("=== End of parameters ==="));
}

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the 
 *              custom device.
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     topic       pointer to topic string
 * @return    combined topic as char pointer, it uses buffer_stca to store the topic
*//*-----------------------------------------------------------------------------------*/
char* build_topic(const char *topic) 
{
  sprintf(buffer_stca, "%s%s", mqttData_sts.dev_short, topic);
  return buffer_stca;
}


/**---------------------------------------------------------------------------------------
 * @brief     This is the setup callback function
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void setupCallback() 
{
  // init the serial
  Serial.begin(115200);
  Serial.println("");
  Serial.println("... starting");
  Serial.print("Firmware Information:");
  Serial.print(FW_IDENTIFIER);
  Serial.println(FW_VERSION);
  Serial.println("Firmware Description:");
  Serial.println(FW_DESCRIPTION);
  Serial.print("Dev ");
  Serial.println(mqttData_sts.dev_short);
  EEPROM.begin(512); // can be up to 4096

  // start wifi manager
  Serial.println();
  Serial.println();
  wifiManager_sts.setAPCallback(configModeCallback);
  wifiManager_sts.setSaveConfigCallback(saveConfigCallback);
  wifiManager_sts.setConfigPortalTimeout(MAX_AP_TIME);
  WiFi.mode(WIFI_STA); // avoid station and ap at the same time

  Serial.println("[WiFi] Connecting ");
  if(!wifiManager_sts.autoConnect(CONFIG_SSID)){
    // possible situataion: Main power out, ESP went to config mode as the routers wifi wasn available on time .. 
    Serial.println("failed to connect and hit timeout, restarting ..");
    delay(1000); // time for serial to print
    ESP.reset(); // reset loop if not only or configured after 5min .. 
  }

  // load all paramters!
  loadConfig();

  InitializePins();
  
  Serial.println("");
  Serial.println("[WiFi] connected");
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client_sts.setServer(mqttData_sts.server_ip, atoi(mqttData_sts.server_port));
  client_sts.setCallback(callback);
}

/**---------------------------------------------------------------------------------------
 * @brief     This callback function handles the loop function
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void loopCallback() 
{

	if (!client_sts.connected()) 
	{
		reconnect();
 	}
 	client_sts.loop();

	//// publish requests ////
  if(millis()-timerLastPub_u32st > PUBLISH_TIME_OFFSET)
  {
    processPublishRequests();
    timerRepubAvoid_u32st = millis();
    timerLastPub_u32st = millis();
  }
  //// publish requests ////

	/// see if we hold down the button for more then 6sec /// 
	if((counterButton_u8st >= 10 && millis() - timerButtonDown_u32st > BUTTON_TIMEOUT) || (true == startWifiConfig_bolst))
	{
    startWifiConfig_bolst = false;
		Serial.println("[SYS] Rebooting to setup mode");
		delay(200);
		wifiManager_sts.startConfigPortal(CONFIG_SSID); // needs to be tested!
		//ESP.reset(); // reboot and switch to setup mode right after that
	}
	/// see if we hold down the button for more then 6sec /// 

}

/**---------------------------------------------------------------------------------------
 * @brief     This is the standard setup function
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void setup() 
{
  setupCallback();
}

/**---------------------------------------------------------------------------------------
 * @brief     This is the standard loop funcktion
 * @author    winkste
 * @date      20 Okt. 2017
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void loop() 
{
  loopCallback();
}


