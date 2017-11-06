/*****************************************************************************************
* FILENAME :        prjsettings.h         
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
#ifndef PRJSETTINGS_H
#define PRJSETTINGS_H

/****************************************************************************************/
/* Imported header files: */

/****************************************************************************************/
/* Global constant defines: */
#define FW_IDENTIFIER             "00001FW" // Firmware identification
#define FW_VERSION                "006"     // Firmware Version
#define FW_DESCRIPTION            "SONOFF BASIC SWITCH"

#define MQTT_SUB_TOGGLE           "/simple_light/toggle" // command message for toggle command
#define MQTT_SUB_BUTTON           "/simple_light/switch" // command message for button commands
#define MQTT_SUB_COMMAND          "/simple_light/cmd" // command message for generic commands
#define MQTT_PUB_LIGHT_STATE      "/simple_light/status" //state of relais
#define MQTT_PAYLOAD_CMD_ON       "ON"
#define MQTT_PAYLOAD_CMD_OFF      "OFF"

//#define BUTTON_INPUT_PIN          0  // D3
#define SIMPLE_LIGHT_PIN          12 // D6
#define LED_PIN                   13 // D7
#define PIR_PIN                   14 // D5
#define PWM_LIGHT_PIN1            4  // D2
#define PWM_LIGHT_PIN2            5  // D1
#define PWM_LIGHT_PIN3            16 // D0
#define DHT_PIN                   2  // D4
#define GPIO_D8                   15 // D8

#define SERIAL_DEBUG

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum, struct, union): */

/****************************************************************************************/
/* Global data allusions (allows type checking, definition in c file): */

/****************************************************************************************/
/* Global function prototypes: */

#endif /* PRJSETTINGS_H */
/****************************************************************************************/
