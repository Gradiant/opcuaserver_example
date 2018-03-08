#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;

//Variable global en la que almacenaremos el número de objetos que detecta el sensor de paso
int32_t numberOfParts = 0;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Recibido ctrl-c");
    running = false;
}

//Función a la que el servidor llamará cada vez que se haga una lectura sobre la variable declarada para almacenar la información del sensor de paso
static UA_StatusCode readInteger(UA_Server * server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId * nodeid, void *nodeContext,
						UA_Boolean sourceTimeStamp,	const UA_NumericRange *range, UA_DataValue *dataValue) {

	dataValue->hasValue = true;
	UA_Int32 myInteger = (UA_Int32) numberOfParts;
	UA_Variant_setScalarCopy(&dataValue->value,&myInteger, &UA_TYPES[UA_TYPES_INT32]);
	return UA_STATUSCODE_GOOD;
}

static void addCounterSensorVariable(UA_Server * server) {

	//En este caso le estamos fijando un ID numérico específico al nodo pero podemos hacer que la librería lo asigne automáticamente.
	UA_NodeId counterNodeId = UA_NODEID_NUMERIC(1, 20305);

	//Fijamos también el nombre que tendrá el nodo OPC-UA
	UA_QualifiedName counterName = UA_QUALIFIEDNAME(1, "Piece Counter[pieces]");

	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.description = UA_LOCALIZEDTEXT("en_US","Piece Counter (units:pieces)");
	attr.displayName = UA_LOCALIZEDTEXT("en_US","Piece Counter");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

	//Fijamos el valor inicial de la variable
	UA_Int32 counterValue = 0;
    UA_Variant_setScalarCopy(&attr.value, &counterValue, &UA_TYPES[UA_TYPES_INT32]);

	//Añadimos la variable al servidor con los parámetros configurados y en el directorio raíz de objetos
	UA_Server_addVariableNode(server, counterNodeId,
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		counterName, UA_NODEID_NULL, attr, NULL, NULL);

}

int main(void) {

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    //Añadimos la variable para el sensor de paso
    addCounterSensorVariable(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
