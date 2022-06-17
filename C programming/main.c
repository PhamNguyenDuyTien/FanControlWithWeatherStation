#include <wiringPi.h>
#include <wiringSerial.h>
#include <wiringPiI2C.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <softPwm.h>
#include "unistd.h"
#include <mysql.h>
#include "MQTTClient.h"

#define SI7021_ADDR                 0x40
#define TEMP_READ_COMMAND			0xE3
#define HUMI_READ_COMMAND			0xE5

#define ADDRESS     "tcp://broker.emqx.io:1883"
#define CLIENTID    "publisher"
#define PUB_TOPIC   "test/sensor"

#define MANUAL                      0
#define AUTOMATIC                   1

#define RED                         0
#define GREEN                       2
#define BLUE                        4

#define BT0                         1
#define BT1                         3

#define IN1                         21
#define IN2                         22
#define ENA                         23

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;

char *server = "localhost";
char *user = "ttt@localhost";
char *password = "1"; 
char *database = "weatherStation";

char msg[40];

int si;
int serial;

uint8_t isUpdated;
uint8_t isUpdatedAuto;

uint8_t data[10];
int PM25_Value, PM10_Value;
int temperature, humidity;

uint8_t buttonStatus = 0;
uint8_t fanMode = AUTOMATIC;
uint8_t fan_level = 0;
uint8_t previousFanLevel = 0;

int8_t range1;
int8_t range2;
int8_t range3;

uint8_t firstRun = 1;

int8_t SI7021_getTemperature()
{
    uint8_t low, high;
    int temperature;

    high = wiringPiI2CReadReg8(si, TEMP_READ_COMMAND);
    low = wiringPiI2CReadReg8(si, TEMP_READ_COMMAND);
    temperature = 175.72*((high << 8) | low)/65536 - 46.85;

    return temperature;
}

uint8_t SI7021_getHumidity()
{
    uint8_t low, high;
    int humidity;

    high = wiringPiI2CReadReg8(si, HUMI_READ_COMMAND);
    low = wiringPiI2CReadReg8(si, HUMI_READ_COMMAND);
    humidity = 125*((high << 8) | low)/65536 - 6;

    return humidity;
}

void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 1;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
    printf("Message '%s' with delivery token %d delivered\n", payload, token);
}

void L298_Init()
{
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    digitalWrite(IN1, 1);
    digitalWrite(IN2, 0);
    pinMode(ENA, OUTPUT);
    softPwmCreate(ENA, 0, 100);
}

void BT0_Interrupt()    // increase
{
    if (digitalRead(BT0) == 0)
    {
        delay(100);
        if (digitalRead(BT0) == 1)
        {
            if (fan_level < 3)
            {
                fan_level++;
                fanMode = MANUAL;
                buttonStatus = 1;
            }
        }
    }
}

void BT1_Interrupt()    // decrease
{
    if (digitalRead(BT1) == 0)
    {
        delay(100);
        if (digitalRead(BT1) == 1)
        {
            if (fan_level > 0)
            {
                fan_level--;
                fanMode = MANUAL;
                buttonStatus = 1;
            }
        }
    }
}

void Button_Init()
{
    pinMode(BT0, INPUT);
    wiringPiISR(BT0, INT_EDGE_FALLING, BT0_Interrupt);
    pinMode(BT1, INPUT);
    wiringPiISR(BT1, INT_EDGE_FALLING, BT1_Interrupt);
}

void LED_Init()
{
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
}

int main()
{
    wiringPiSetup();
    L298_Init();
    Button_Init();
    LED_Init();

    // ket noi si7020
    si = wiringPiI2CSetup(SI7021_ADDR);
    // ket noi uart
    serial = serialOpen ("/dev/ttyAMA0", 9600);
    // khởi tạo client
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    uint32_t to, t1;

    while (1)
    {
        
            // connect database
        conn = mysql_init(NULL);
        mysql_real_connect(conn, server, user, password, database, 0, NULL, 0);
        char sql[200];
        // read data from database
        sprintf(sql, "select * from control where id = 1");
        mysql_query(conn, sql);
        res = mysql_store_result(conn); 
        while (row = mysql_fetch_row(res)) 
        { 
            fanMode = atoi(row[0]);
            fan_level = atoi(row[1]);
            isUpdated = atoi(row[2]);
            isUpdatedAuto = atoi(row[3]);
        }       
        // update table control
        sprintf(sql, "update control set isUpdated = 0, isUpdatedAuto = 0 where id=1");
        
        mysql_query(conn, sql);
        mysql_free_result(res);  
        mysql_close(conn);

        if(isUpdatedAuto != 0 || firstRun == 1){
            firstRun = 0;
            conn = mysql_init(NULL);
            mysql_real_connect(conn, server, user, password, database, 0, NULL, 0);
            char sql[200];
            // read data from database
            sprintf(sql, "select * from auto where id=1");
            mysql_query(conn, sql);
            res = mysql_store_result(conn); 
            while (row = mysql_fetch_row(res)) 
	        { 
                range1 = atoi(row[1]);
                range2 = atoi(row[2]);
                range3 = atoi(row[3]);
            }


            mysql_query(conn, sql);
            mysql_free_result(res);  
            mysql_close(conn);
        }
        if ((serialDataAvail(serial) != 0))
        {
            
            if(serialGetchar(serial) == 170){
                if (serialGetchar(serial) == 192)
                {
                    for (int i = 0; i < 7; i++)
                    {
                        data[i] = serialGetchar(serial);
                    }
                    serialFlush(serial);

                    PM25_Value = ((data[1] << 8) | data[0])/10;
                    PM10_Value = ((data[3] << 8) | data[2])/10;
                    temperature = SI7021_getTemperature();
                    humidity = SI7021_getHumidity();
                    sprintf(msg, "%d, %d, %d, %d", PM25_Value, PM10_Value, temperature, humidity);
                    publish(client, PUB_TOPIC, msg);
                }
            }
            
        }

        // temperature = SI7021_getTemperature();
        // humidity = SI7021_getHumidity();
        // sprintf(msg, "%d, %d, %d, %d", 1, 1, temperature, humidity);
        // publish(client, PUB_TOPIC, msg);
        if (fanMode == AUTOMATIC)
        {
            printf("Range1: %d - Range2: %d - Range3: %d\n", range1, range2, range3);
            printf("fanMode: %d\n", fanMode);
            printf("Temp: %d\n", temperature);
            if (temperature < range1)
            {
                softPwmWrite(ENA, 0);
                digitalWrite(GREEN, 0);
                digitalWrite(BLUE, 0);
                digitalWrite(RED, 0);
                fan_level = 0;
            }
            else if ((temperature >= range1) && (temperature < range2))
            {
                softPwmWrite(ENA, 40);
                digitalWrite(GREEN, 0);
                digitalWrite(BLUE, 0);
                digitalWrite(RED, 1);
                fan_level = 1;
            }
            else if ((temperature >= range2) && (temperature < range3)) 
            {
                softPwmWrite(ENA, 70);
                digitalWrite(RED, 0);
                digitalWrite(BLUE, 0);
                digitalWrite(GREEN, 1);
                fan_level = 2;
            }
            else if (temperature >= range3) 
            {
                softPwmWrite(ENA, 100);
                softPwmWrite(ENA, 100);
                digitalWrite(RED, 0);
                digitalWrite(GREEN, 0);
                digitalWrite(BLUE, 1);
                fan_level = 3;
            }

            if (previousFanLevel != fan_level)
            {
                // updated database
                conn = mysql_init(NULL);
                if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) 
                {
                    fprintf(stderr, "%s\n", mysql_error(conn));
                    mysql_close(conn);
                    exit(1);
                }  
                char sql[200];
                sprintf(sql,"update control set fanMode = %d, fanLevel = %d", fanMode, fan_level);
                mysql_query(conn,sql);
                mysql_close(conn);

                previousFanLevel = fan_level;
            }
        }
        else
        {
            if ((buttonStatus == 1) || (isUpdated == 1))
            {
                switch (fan_level)
                {
                    case 0:
                        softPwmWrite(ENA, 0);
                        digitalWrite(GREEN, 0);
                        digitalWrite(BLUE, 0);
                        digitalWrite(RED, 0);
                        break;
                    case 1:
                        softPwmWrite(ENA, 40);
                        digitalWrite(GREEN, 0);
                        digitalWrite(BLUE, 0);
                        digitalWrite(RED, 1);
                        break;
                    case 2:
                        softPwmWrite(ENA, 70);
                        digitalWrite(RED, 0);
                        digitalWrite(BLUE, 0);
                        digitalWrite(GREEN, 1);
                        break;
                    case 3:
                        softPwmWrite(ENA, 100);
                        digitalWrite(RED, 0);
                        digitalWrite(GREEN, 0);
                        digitalWrite(BLUE, 1);
                        break;
                }

                if (buttonStatus == 1)
                {
                    // updated database
                    conn = mysql_init(NULL);
                    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) 
                    {
                        fprintf(stderr, "%s\n", mysql_error(conn));
                        mysql_close(conn);
                        exit(1);
                    }  
                    char sql[200];
                    sprintf(sql,"update control set fanMode = %d, fanLevel = %d", fanMode, fan_level);
                    mysql_query(conn,sql);
                    mysql_close(conn);
                    // clear buttonStatus
                    buttonStatus = 0;
                }

                isUpdated = 0;
            }
        }
    }

    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);

    return rc;
}

/* gcc main.c -o main -l wiringPi -lpaho-mqtt3c $(mariadb_config --cflags) $(mariadb_config --libs)*/
