#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "open62541.h"

//Definimos las rutas relevantes para la configuración del GPIO
#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_FILENAME "/sys/class/gpio/gpio60/value"

//Definimos el tiempo de muestreo del valor del sensor
#define SLEEP_TIME_MILLIS 50

//Definimos el ID del nodo externamente ya que nos hará falta en el thread
#define COUNTER_NODE_ID 20305

UA_Boolean running = true;

int32_t numberOfParts = 0;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Recibido ctrl-c");
    running = false;
}

//Esta función permite exportar el GPIO inicialmente. Sólo sería necesario hacerlo al iniciar la placa pero la ejecutaremos en cualquier caso.
void configureGpio() {

	FILE *fp;
	if (fp = fopen(GPIO_EXPORT, "w")){
		fprintf(fp,"60");
		fclose(fp);
	} else {
		printf("No se ha podido exportar el GPIO\n");
	}

}

//Para poder monitorizar el sensor de paso en paralelo crearemos un thread.
void *mainSensor(void *ptr) {

	UA_Server * server = ptr;

	int utime = SLEEP_TIME_MILLIS * 1000;
	int currentChar = 0;
	bool valueIsZero = false;
	FILE *fp;

	//Este código actualiza el contador cada vez que el valor del fichero del GPIO pasa de 1 a 0.
	while (running == 1){
		if (fp = fopen(GPIO_FILENAME, "r")){
			currentChar = fgetc(fp);
			if (currentChar == 48){
				if (valueIsZero == false) {
					numberOfParts = numberOfParts + 1;
					printf("Actualizado contador: %i\n", numberOfParts);

					//Actualizamos el valor del nodo OPC-UA
					UA_Variant value;
					UA_Int32 myInteger = (UA_Int32) numberOfParts;
					UA_Variant_setScalarCopy(&value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
					UA_Server_writeValue(server, UA_NODEID_NUMERIC(1,COUNTER_NODE_ID), value);

					valueIsZero = true;
				}
			} else {
				valueIsZero = false;
			}
			fclose(fp);
		} else {
			printf("No se ha podido leer el GPIO\n");
		}
		usleep(utime);
	}

}

static void addCounterSensorVariable(UA_Server * server) {

	UA_NodeId counterNodeId = UA_NODEID_NUMERIC(1, COUNTER_NODE_ID);

	UA_QualifiedName counterName = UA_QUALIFIEDNAME(1, "Piece Counter[pieces]");

	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.description = UA_LOCALIZEDTEXT("en_US","Piece Counter (units:pieces)");
	attr.displayName = UA_LOCALIZEDTEXT("en_US","Piece Counter");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

	UA_Int32 counterValue = 0;
    UA_Variant_setScalarCopy(&attr.value, &counterValue, &UA_TYPES[UA_TYPES_INT32]);

	UA_Server_addVariableNode(server, counterNodeId,
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		counterName, UA_NODEID_NULL, attr, NULL, NULL);

}

int main(void) {

	int ret;
	pthread_t threadSensor;

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	configureGpio();

	addCounterSensorVariable(server);

	//Lanzamos el thread para monitorizar el sensor. Le pasamos el servidor OPC-UA como parámetro para poder actualizar el valor del nodo.
	if(pthread_create( &threadSensor, NULL, mainSensor, server)) {
		fprintf(stderr,"Error - pthread_create(): %d\n",ret);
		exit(EXIT_FAILURE);
	}

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
