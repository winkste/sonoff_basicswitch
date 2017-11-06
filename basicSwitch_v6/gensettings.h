/*****************************************************************************************
* FILENAME :        version.h         
*
* DESCRIPTION :
*       application based settings 
*
* NOTES :...
*       
*
*       Copyright A.N.Other Co. 2017.  All rights reserved.
* 
* AUTHOR :    Stephan Wink        START DATE :    01.10.2017
*
*
* REF NO  VERSION DATE    WHO     DETAIL
* 000       16.10         SWI     First working version      
*
*****************************************************************************************/
#ifndef GENSETTINGS_H
#define GENSETTINGS_H

/****************************************************************************************/
/* Imported header files: */

/****************************************************************************************/
/* Global constant defines: */
#define CONFIG_SSID               "OPEN_ESP_CONFIG_AP2" // SSID of the configuration mode
#define MAX_AP_TIME               300 // restart eps after 300 sec in config mode

//#define MSG_BUFFER_SIZE           60  // mqtt messages max char size
#define MQTT_DEFAULT_DEVICE       "devXX" // default room device 

#define MQTT_PUB_FW_IDENT         "/simple_light/fwident" //firmware identification
#define MQTT_PUB_FW_VERSION       "/simple_light/fwversion" //firmware version
#define MQTT_PUB_FW_DESC          "/simple_light/desc" //firmware description
#define MQTT_CLIENT               MQTT_DEFAULT_DEVICE // just a name used to talk to MQTT broker
#define MQTT_PAYLOAD_CMD_INFO     "INFO"
#define MQTT_PAYLOAD_CMD_SETUP    "SETUP"
#define PUBLISH_TIME_OFFSET       200     // ms timeout between two publishes

#define BUTTON_INPUT_PIN          0  // D3
#define BUTTON_TIMEOUT            1500  // max 1500ms timeout between each button press to count up (start of config)
#define BUTTON_DEBOUNCE           400  // ms debouncing for the botton

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum, struct, union): */
// Buffer to hold data from the WiFi manager for mqtt login
typedef struct mqttData_tag{ //80 byte
  char login[16];
  char pw[16];
  char dev_short[6];
  char cap[2]; // capability
  char server_ip[16];
  char server_port[6];
}mqttData_t;

/****************************************************************************************/
/* Global data allusions (allows type checking, definition in c file): */

/****************************************************************************************/
/* Global function prototypes: */

#endif /* GENSETTINGS_H */
/****************************************************************************************/
