/**
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>

#include "lorawan/LoRaWANInterface.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "events/EventQueue.h"
#include "./MBed_Adafruit_GPS.h"
#include "./Si7021.h"
#include "./SEN_13322.h"
#include "./HW5P1_2015.h"
#include "./RGBLED.h"
#include "./TCS3472_I2C.h"	
#include "./MMA8451Q.h"

// Application helpers
#include "trace_helper.h"
#include "lora_radio_helper.h"

using namespace events;

// Max payload size can be LORAMAC_PHY_MAXPAYLOAD.
// This example only communicates with much shorter messages (<30 bytes).
// If longer messages are used, these buffers must be changed accordingly.
uint8_t tx_buffer[30];
uint8_t rx_buffer[30];

/*
 * Sets up an application dependent transmission timer in ms. Used only when Duty Cycling is off for testing
 */
#define TX_TIMER                        10000

/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
 * Maximum number of retries for CONFIRMED messages before giving up
 */
#define CONFIRMED_MSG_RETRY_COUNTER     3

/**
 * Dummy pin for dummy sensor
 */
#define PC_9                            0

/**
 * Dummy sensor class object
 */
DS1820  ds1820(PC_9);

/**
* This event queue is the global event queue for both the
* application and stack. To conserve memory, the stack is designed to run
* in the same thread as the application and the application is responsible for
* providing an event queue to the stack that will be used for ISR deferment as
* well as application information event queuing.
*/
static EventQueue ev_queue(MAX_NUMBER_OF_EVENTS *EVENTS_EVENT_SIZE);

/**
 * Event handler.
 *
 * This will be passed to the LoRaWAN stack to queue events for the
 * application which in turn drive the application.
 */
static void lora_event_handler(lorawan_event_t event);

/**
 * Constructing Mbed LoRaWANInterface and passing it the radio object from lora_radio_helper.
 */
static LoRaWANInterface lorawan(radio);

/**
 * Application specific callbacks
 */
static lorawan_app_callbacks_t callbacks;
static uint8_t DEV_EUI[] = { 0x83, 0x39, 0x32, 0x35, 0x59, 0x37, 0x91, 0x94 };
static uint8_t APP_EUI[] = { 0x70, 0xb3, 0xd5, 0x7e, 0xd0, 0x00, 0xfc, 0xda };
static uint8_t APP_KEY[] = { 0xf3,0x1c,0x2e,0x8b,0xc6,0x71,0x28,0x1d,0x51,0x16,0xf0,0x8f,0xf0,0xb7,0x92,0x8f };
/**
 * Entry point for application
 */

BufferedSerial* gps_Serial = new BufferedSerial(PA_9, PA_10,9600); 
Thread gps_thread(osPriorityNormal, 2048);
Adafruit_GPS myGPS(gps_Serial); 
TCS3472_I2C rgbSensor(PB_9, PB_8);

float latitude, longitude;
Mutex mutex;

void readGps() {
	char c; //when read via Adafruit_GPS::read(), the class returns single character stored here
	//Timer refresh_Timer; //sets up a timer for use in loop; how often do we print GPS info?
	//const int refresh_Time = 2000; //refresh time in ms
	while(true) {
			//refresh_Timer.start(); 
			c = myGPS.read();   //queries the GPS
			//if (c) { printf("%c", c); } //this line will echo the GPS data if not paused
			//check if we recieved a new message from GPS, if so, attempt to parse it,
			if ( myGPS.newNMEAreceived() ) {
				if (!myGPS.parse(myGPS.lastNMEA())) {
					continue;
				}
			}
			
			mutex.lock();
		  latitude = myGPS.latitude / 100.0;
			longitude = myGPS.longitude / 100.0;
			if (myGPS.lat == 'S') {
				latitude = -latitude;
			} 
			if (myGPS.lon == 'W') {
				longitude = -longitude;
			}
			if (latitude == 0.0) {
				latitude = 45.23234;
			}
			if (longitude == 0.0) {
				longitude = -2.27098;
			}
			mutex.unlock();
		}
}

int main(void)
{
		gps_thread.start(readGps);
		
		rgbSensor.enablePowerAndRGBC();
    // setup tracing
    setup_trace();

    // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;
		
    // Initialize LoRaWAN stack
    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        printf("\r\n LoRa initialization failed! \r\n");
        return -1;
    }

    printf("\r\n Mbed LoRaWANStack initialized \r\n");

    // prepare application callbacks
    callbacks.events = mbed::callback(lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        printf("\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        return -1;
    }

    printf("\r\n CONFIRMED message retries : %d \r\n",
           CONFIRMED_MSG_RETRY_COUNTER);

    // Enable adaptive data rate
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        printf("\r\n enable_adaptive_datarate failed! \r\n");
        return -1;
    }

    printf("\r\n Adaptive data  rate (ADR) - Enabled \r\n");
    lorawan_connect_t connect_params;
		connect_params.connect_type = LORAWAN_CONNECTION_OTAA;
    connect_params.connection_u.otaa.dev_eui = DEV_EUI;
    connect_params.connection_u.otaa.app_eui = APP_EUI;
    connect_params.connection_u.otaa.app_key = APP_KEY;
    connect_params.connection_u.otaa.nb_trials = 3;
		
    retcode = lorawan.connect(connect_params);

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("\r\n Connection error, code = %d \r\n", retcode);
        return -1;
    }

    printf("\r\n Connection - In Progress ...\r\n");

    // make your event queue dispatching events forever
    ev_queue.dispatch_forever();

    return 0;
}

/**
 * Sends a message to the Network Server
 */
static void send_message()
{
	  Si7021 tempHumSensor(PB_9,PB_8);
		SEN_13322 soilMoistureSensor(PA_0);
		HW5P1_2015 lightSensor(PA_4);
    MMA8451Q acc(PB_9,PB_8,0x1d<<1);
	
	  float accValues[3];
    uint16_t packet_len;
    int16_t retcode;
		uint16_t rgbValues[4];
    
		
		tempHumSensor.measure();
		rgbSensor.getAllColors(rgbValues);
		acc.getAllAxis(accValues);
	
		uint16_t temperature = (uint16_t)tempHumSensor.get_temperature();
		uint16_t humidity = (uint16_t)tempHumSensor.get_humidity();
	  uint16_t light = (uint16_t)lightSensor.readLight();
		uint16_t soilMoisture = (uint16_t)soilMoistureSensor.getMoistureValue();
		
		printf("Temperature: %d\n", temperature);
		printf("Humidity: %d\n", humidity);
	  printf("Light: %d\n", light);
		printf("Soil moisture: %d\n", soilMoisture);
	  printf("RED: %d\n", rgbValues[1]);
		printf("GREEN: %d\n", rgbValues[2]);
		printf("BLUE: %d\n", rgbValues[3]);
		printf("ACC Z AXIS: %3.2f\n", accValues[2]);
		printf("ACC Y AXIS: %3.2f\n", accValues[1]);
		printf("LOCATION: %5.2f, %5.2f", latitude, longitude);

		memcpy(tx_buffer, &latitude, sizeof(float));
    memcpy(tx_buffer + 4, &longitude, sizeof(float));
    memcpy(tx_buffer + 8, &temperature, sizeof(uint16_t));
		memcpy(tx_buffer + 10, &humidity, sizeof(uint16_t));
		memcpy(tx_buffer + 12, &light, sizeof(uint16_t));
		memcpy(tx_buffer + 14, &soilMoisture, sizeof(uint16_t));
		memcpy(tx_buffer + 16, &rgbValues[1], sizeof(uint16_t));
		memcpy(tx_buffer + 18, &rgbValues[2], sizeof(uint16_t));
		memcpy(tx_buffer + 20, &rgbValues[3], sizeof(uint16_t));
		memcpy(tx_buffer + 22, &accValues[2], sizeof(float));
		memcpy(tx_buffer + 26, &accValues[1], sizeof(float));
		
		for (int i = 0; i<30; i++) {
			printf("%02x", tx_buffer[i]);
		}
		printf("\n");
	  int packetLen = 4 * sizeof(float) + 7 * sizeof(uint16_t);
    retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packetLen,
                           MSG_UNCONFIRMED_FLAG);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - WOULD BLOCK\r\n")
        : printf("\r\n send() - Error code %d \r\n", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                ev_queue.call_in(3000, send_message);
            }
        }
        return;
    }

    printf("\r\n %d bytes scheduled for transmission \r\n", retcode);
    memset(tx_buffer, 0, sizeof(tx_buffer));
}

/**
 * Receive a message from the Network Server
 */
static void receive_message()
{
	  RGBLED rgbLed(PH_0, PB_13, PH_1);
    uint8_t port;
    int flags;
    int16_t retcode = lorawan.receive(rx_buffer, sizeof(rx_buffer), port, flags);

    if (retcode < 0) {
        printf("\r\n receive() - Error code %d \r\n", retcode);
        return;
    }

    printf(" RX Data on port %u (%d bytes): ", port, retcode);
    for (uint8_t i = 0; i < retcode; i++) {
        printf("%02x ", rx_buffer[i]);
    }
    printf("\r\n");
    rgbLed.setColor((char*)rx_buffer);
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

/**
 * Event handler
 */
static void lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            printf("\r\n Connection - Successful \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            } else {
                ev_queue.call_every(TX_TIMER, send_message);
            }

            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("\r\n Disconnected Successfully \r\n");
            break;
        case TX_DONE:
            printf("\r\n Message Sent to Network Server \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("\r\n Transmission Error - EventCode = %d \r\n", event);
            // try again
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case RX_DONE:
            printf("\r\n Received message from Network Server \r\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("\r\n Error in reception - Code = %d \r\n", event);
            break;
        case JOIN_FAILURE:
            printf("\r\n OTAA Failed - Check Keys \r\n");
            break;
        case UPLINK_REQUIRED:
            printf("\r\n Uplink required by NS \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

// EOF
