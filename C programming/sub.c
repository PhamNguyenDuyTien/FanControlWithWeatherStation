#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "MQTTClient.h"
#include <mysql.h>

#define ADDRESS     "tcp://broker.emqx.io:1883"
#define CLIENTID    "subscriber"
#define SUB_TOPIC   "test/sensor"

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;

char *server = "localhost";
char *user = "ttt@localhost";
char *password = "1"; 
char *database = "weatherStation";

int PM10_Value, PM25_Value, temperature, humidity;
/*
int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) 
{
    char* payload = message->payload;
    printf("Received message: %s\n", payload);
    char *tok;

    tok = payload;
    tok = strtok(NULL, "-");
    PM25_Value = atoi(tok);
    tok = strtok(tok, "-");
    PM10_Value = atoi(tok);
    tok = strtok(NULL, "-");
    temperature = atoi(tok);
    tok = strtok(NULL, "-");
    humidity = atoi(tok);
    printf("%d-%d-%d-%d\n", PM25_Value, PM10_Value, temperature, humidity);

    // printf("%d",PM10_Value);
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }  
    char sql[200];
    sprintf(sql,"insert into parameters(pm25,pm10,temp,humid) values (PM25_Value,PM10_Value,temperature,humidity)");
    // => sua dong nay
    mysql_query(conn,sql);
    mysql_close(conn);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
*/

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) 
{
    char* payload = message->payload;
    printf("Received message: %s\n", payload);


    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }  
    char sql[200];
    //sprintf(sql,"insert into parameters(pm25,pm10,temp,humid) values (PM25_Value,PM10_Value,temperature,humidity)");
    sprintf(sql,"insert into parameters(pm25,pm10,temp,humid) values (%s)", payload);
    
    mysql_query(conn,sql);
    mysql_close(conn);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);

    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    
    //listen for operation
    MQTTClient_subscribe(client, SUB_TOPIC, 0);

    while(1);
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    return rc;
}

/* gcc sub.c -o sub -l wiringPi -lpaho-mqtt3c $(mariadb_config --cflags) $(mariadb_config --libs) */
