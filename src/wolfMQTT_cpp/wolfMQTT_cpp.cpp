/* 
 * Copyright © 2025, UChicago Argonne, LLC
 * All Rights Reserved
 * Software Name: Remote Test Harness
 * By: Argonne National Laboratory
 * 
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 * Copyright © 2007 Free Software Foundation, Inc. <https://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
 * 
 * See the LICENSE file for the full license text.
 */


//=====================================================================================================================
// wolfMQTT_cpp.cpp - Implementation of an MQTT client as a wrapper around wolfMQTT library
//=====================================================================================================================

#include <map>
#include <thread>
#include "wolfMQTT_cpp.h"

// Declare a static map pointing the client object to the base class
static std::map<MqttClient*, CWolfMQTTBase*> object_map;

// -----------------------------------------------------------------------------
// CWolfMQTTBase() - constructor
// -----------------------------------------------------------------------------
CWolfMQTTBase::CWolfMQTTBase()
{
    // Initialize socket descriptor for MQTT connection context
    m_sd = -1;

    // Initialize pipe values
    m_pipe[0]=m_pipe[1]=-1;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// callback() - This is the callback function for incoming messages received when subscribed to a topic
// -----------------------------------------------------------------------------
static int callback(MqttClient *client, MqttMessage *msg,
    unsigned char msg_new, unsigned char msg_done)
{
    // Extract topic name and payload and make sure they're null terminated
    char message_buf[256];
    char topic_buf[256];
    
    memcpy(topic_buf, msg->topic_name, msg->topic_name_len);
    topic_buf[msg->topic_name_len] = '\0';

    memcpy(message_buf, msg->buffer, msg->buffer_len);
    message_buf[msg->buffer_len] = '\0';

    // Convert topic and message to strings
    std::string topic = topic_buf;
    std::string message = message_buf;

    // Retrieve the pointer to the MqttClient we care about
    CWolfMQTTBase* object = object_map[client];

    // Now invoke the appropriate message handler for the class instance
    object->handle_message(topic, message);

    // Return negative to terminate publish processing
    return MQTT_CODE_SUCCESS;
}
// -----------------------------------------------------------------------------



//==============================================================================
// The following are support functions from wolfMQTT library
//==============================================================================

// -----------------------------------------------------------------------------
// setup_timeout
// -----------------------------------------------------------------------------
static void setup_timeout(struct timeval* tv, int timeout_ms)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;

    /* Make sure there is a minimum value specified */
    if (tv->tv_sec < 0 || (tv->tv_sec == 0 && tv->tv_usec <= 0)) {
        tv->tv_sec = 0;
        tv->tv_usec = 100;
    }
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// socket_get_error
// -----------------------------------------------------------------------------
static int socket_get_error(int sockFd)
{
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    return so_error;
}
// -----------------------------------------------------------------------------




// -----------------------------------------------------------------------------
// mqtt_net_connect
// -----------------------------------------------------------------------------
static int mqtt_net_connect(void *context, const char* host, word16 port,
    int timeout_ms)
{
    int rc;
    int sockFd, *pSockFd = (int*)context;
    struct sockaddr_in addr;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    (void)timeout_ms;

    /* get address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    rc = getaddrinfo(host, NULL, &hints, &result);
    if (rc >= 0 && result != NULL) {
        struct addrinfo* res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }
        if (result->ai_family == AF_INET) {
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            addr.sin_addr =
                ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
        }
        else {
            rc = -1;
        }
        freeaddrinfo(result);
    }
    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    sockFd = socket(addr.sin_family, SOCK_STREAM, 0);
    if (sockFd < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Start connect */
    rc = connect(sockFd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc < 0) {
        // printf("NetConnect: Error %d (Sock Err %d)\n",
            // rc, socket_get_error(*pSockFd));
        close(sockFd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* save socket number to context */
    *pSockFd = sockFd;

    return MQTT_CODE_SUCCESS;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// mqtt_net_read
// -----------------------------------------------------------------------------
static int mqtt_net_read(void *context, byte* buf, int buf_len, int timeout_ms)
{
    int rc;
    int *pSockFd = (int*)context;
    int bytes = 0;
    struct timeval tv;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(*pSockFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

    /* Loop until buf_len has been read, error or timeout */
    while (bytes < buf_len) {
        rc = (int)recv(*pSockFd, &buf[bytes], buf_len - bytes, 0);
        if (rc < 0) {
            rc = socket_get_error(*pSockFd);
            if (rc == 0)
                break; /* timeout */
            // PRINTF("NetRead: Error %d", rc);
            return MQTT_CODE_ERROR_NETWORK;
        }
        bytes += rc; /* Data */
    }

    if (bytes == 0) {
        return MQTT_CODE_ERROR_TIMEOUT;
    }

    return bytes;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// mqtt_net_write
// -----------------------------------------------------------------------------
static int mqtt_net_write(void *context, const byte* buf, int buf_len,
    int timeout_ms)
{
    int rc;
    int *pSockFd = (int*)context;
    struct timeval tv;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(*pSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    rc = (int)send(*pSockFd, buf, buf_len, 0);
    if (rc < 0) {
        // printf("NetWrite: Error %d (Sock Err %d)\n",
        //     rc, socket_get_error(*pSockFd));
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// mqtt_net_disconnect
// -----------------------------------------------------------------------------
static int mqtt_net_disconnect(void *context)
{
    int *pSockFd = (int*)context;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    close(*pSockFd);
    *pSockFd = -1;

    return MQTT_CODE_SUCCESS;
}
// -----------------------------------------------------------------------------




#ifdef ENABLE_MQTT_TLS
// -----------------------------------------------------------------------------
// mqtt_tls_verify_cb
// -----------------------------------------------------------------------------
static int mqtt_tls_verify_cb(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    char buffer[WOLFSSL_MAX_ERROR_SZ];
    printf("MQTT TLS Verify Callback: PreVerify %d, Error %d (%s)\n",
        preverify, store->error, store->error != 0 ?
            wolfSSL_ERR_error_string(store->error, buffer) : "none");
    printf("  Subject's domain name is %s\n", store->domain);

    if (store->error != 0)
    {
        /* Allowing to continue */
        /* Should check certificate and return 0 if not okay */
        printf("  Allowing cert anyways\n");
    }

    return 1;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// mqtt_tls_cb - Use this callback to setup TLS certificates and verify callbacks
// -----------------------------------------------------------------------------
static int mqtt_tls_cb(MqttClient* client)
{
    int rc = WOLFSSL_FAILURE;

    /* Use highest available and allow downgrade. If wolfSSL is built with
     * old TLS support, it is possible for a server to force a downgrade to
     * an insecure version. */
    client->tls.ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (client->tls.ctx)
    {
        wolfSSL_CTX_set_verify(client->tls.ctx, WOLFSSL_VERIFY_PEER,
                               mqtt_tls_verify_cb);

        /* default to success */
        rc = WOLFSSL_SUCCESS;

        #if !defined(NO_CERT)
            #if 0
                /* Load CA certificate buffer */
                rc = wolfSSL_CTX_load_verify_buffer(client->tls.ctx, caCertBuf,
                                                caCertSize, WOLFSSL_FILETYPE_PEM);
            #endif
            #if 0
                /* If using a client certificate it can be loaded using: */
                rc = wolfSSL_CTX_use_certificate_buffer(client->tls.ctx,
                                clientCertBuf, clientCertSize, WOLFSSL_FILETYPE_PEM);
            #endif
        #endif /* !NO_CERT */
    }
    
    // printf("MQTT TLS Setup (%d)\n", rc);

    return rc;
}
// -----------------------------------------------------------------------------


#else
// -----------------------------------------------------------------------------
// mqtt_tls_cb
// -----------------------------------------------------------------------------

static int mqtt_tls_cb(MqttClient* client)
{
    (void)client;
    return 0;
}
// -----------------------------------------------------------------------------
#endif /* ENABLE_MQTT_TLS */




// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
static word16 mqtt_get_packetid(void)
{
    static volatile word16 mPacketIdLast;

    /* Check rollover */
    if (mPacketIdLast >= MAX_PACKET_ID) {
        mPacketIdLast = 0;
    }

    return ++mPacketIdLast;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// init() - Call this function before calling connect to change default MQTT settings if needed
// -----------------------------------------------------------------------------

void CWolfMQTTBase::init(MqttQoS _MQTT_QOS, int _MQTT_KEEP_ALIVE_SEC, int _MQTT_CMD_TIMEOUT_MS, int _MQTT_CON_TIMEOUT_MS, int _MQTT_USE_TLS, int _PRINT_BUFFER_SIZE, int _MQTT_MAX_PACKET_SIZE)
{
    MQTT_QOS             = _MQTT_QOS;
    MQTT_KEEP_ALIVE_SEC  = _MQTT_KEEP_ALIVE_SEC;
    MQTT_CMD_TIMEOUT_MS  = _MQTT_CMD_TIMEOUT_MS;
    MQTT_CON_TIMEOUT_MS  = _MQTT_CON_TIMEOUT_MS;
    MQTT_USE_TLS         = _MQTT_USE_TLS;
    PRINT_BUFFER_SIZE    = _PRINT_BUFFER_SIZE;
    MQTT_MAX_PACKET_SIZE = _MQTT_MAX_PACKET_SIZE;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// connect() - Call this to connect to an MQTT broker
// -----------------------------------------------------------------------------
int CWolfMQTTBase::connect(std::string ip, int port, std::string username, std::string password, std::string client_id)
{
    char one = 1;
    byte m_SendBuf[MQTT_MAX_PACKET_SIZE];
    byte m_ReadBuf[MQTT_MAX_PACKET_SIZE];

    // If it doesn't exist, create the pipe that the threads will use to communicate
    if (m_pipe[0] == -1)
    {
        pipe2(m_pipe, O_CLOEXEC);
    
        // Now create the thread that executes the wolfMQTT statemachine 
        // which waits for messages to arrive on subscribed topics
        // and calls the callback function
        std::thread th(&CWolfMQTTBase::wolfMQTT_state_machine, this);
        th.detach();
    }

    // Define all functions
    memset(&m_network, 0, sizeof(m_network));
    m_network.connect = mqtt_net_connect;
    m_network.read = mqtt_net_read;
    m_network.write = mqtt_net_write;
    m_network.disconnect = mqtt_net_disconnect;
    m_network.context = &m_sd;


    // Initialize MQTT client
    int rc = MqttClient_Init(&mClient, &m_network, callback,
        m_SendBuf, sizeof(m_SendBuf), m_ReadBuf, sizeof(m_ReadBuf),
        MQTT_CON_TIMEOUT_MS);
    if (rc != MQTT_CODE_SUCCESS) return rc;
        // goto exit;
        // printf("error\n");


    // Connect to MQTT broker
    rc = MqttClient_NetConnect(&mClient, ip.c_str(), port,
        MQTT_CON_TIMEOUT_MS, MQTT_USE_TLS, mqtt_tls_cb);
    if (rc != MQTT_CODE_SUCCESS) return rc;
        // goto exit;
    //     printf("error\n");
    // printf("MQTT Network Connect Success: Host %s, Port %d, UseTLS %d\n",
    //     ip.c_str(), port, MQTT_USE_TLS);


    // Send Connect and wait for Ack
    memset(&mqttObj, 0, sizeof(mqttObj));
    mqttObj.connect.keep_alive_sec = MQTT_KEEP_ALIVE_SEC;
    mqttObj.connect.client_id = client_id.c_str();
    mqttObj.connect.username = username.c_str();
    mqttObj.connect.password = password.c_str();
    rc = MqttClient_Connect(&mClient, &mqttObj.connect);
    if (rc != MQTT_CODE_SUCCESS) return rc;
        // goto exit;
        // printf("error\n");
    // printf("MQTT Broker Connect Success: ClientID %s, Username %s, Password %s\n",
    //     client_id.c_str(),
    //     (username.c_str() == NULL) ? "Null" : username.c_str(),
    //     (password.c_str() == NULL) ? "Null" : password.c_str());

    // Insert address of MqttClient object in the map pointing to instance of this class
    object_map[&mClient] = this;
    
    // Tell the wolfMQTT_state_machine thread that we're all set up
    write(m_pipe[1], &one, 1);

    return MQTT_CODE_SUCCESS;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// subscribe() - Call this to subscribe to an MQTT topic on the broker
// -----------------------------------------------------------------------------
int CWolfMQTTBase::subscribe(std::string topic)
{
    // Subscribe to topic and wait for Ack
    MqttTopic topics[1];
    memset(&mqttObj, 0, sizeof(mqttObj));
    topics[0].topic_filter = topic.c_str();
    topics[0].qos = MQTT_QOS;
    mqttObj.subscribe.packet_id = mqtt_get_packetid();
    mqttObj.subscribe.topic_count = sizeof(topics) / sizeof(MqttTopic);
    mqttObj.subscribe.topics = topics;
    int rc = MqttClient_Subscribe(&mClient, &mqttObj.subscribe);

    if (rc != MQTT_CODE_SUCCESS) return rc;

        // goto exit;
        // printf("error\n");

        return rc;
    
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// unsubscribe() - Call this to unsubscribe to an MQTT topic on the broker
// -----------------------------------------------------------------------------
int CWolfMQTTBase::unsubscribe(std::string topic)
{
    // Unsubscribe to topic and wait for Ack
    MqttTopic topics[1];
    memset(&mqttObj, 0, sizeof(mqttObj));
    topics[0].topic_filter = topic.c_str();
    topics[0].qos = MQTT_QOS;
    mqttObj.unsubscribe.packet_id = mqtt_get_packetid();
    mqttObj.unsubscribe.topic_count = sizeof(topics) / sizeof(MqttTopic);
    mqttObj.unsubscribe.topics = topics;
    int rc = MqttClient_Unsubscribe(&mClient, &mqttObj.unsubscribe);
    if (rc != MQTT_CODE_SUCCESS) return rc;
        // goto exit;
    //     printf("error\n");
    // printf("MQTT Unsubscribe Success: Topic %s, QoS %d\n",
    //     topic.c_str(), MQTT_QOS);

    return rc;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// publish() - Call this to publish a message on the given topic
// -----------------------------------------------------------------------------
int CWolfMQTTBase::publish(std::string topic, std::string message)
{
    memset(&mqttObj, 0, sizeof(mqttObj));
    mqttObj.publish.qos = MQTT_QOS;
    mqttObj.publish.topic_name = topic.c_str();
    mqttObj.publish.packet_id = mqtt_get_packetid();
    mqttObj.publish.buffer = (byte*)message.c_str();
    mqttObj.publish.total_len = XSTRLEN(message.c_str());
    int rc = MqttClient_Publish(&mClient, &mqttObj.publish);
    if (rc != MQTT_CODE_SUCCESS) return rc;
        // goto exit;
    //     printf("error\n");
    // printf("MQTT Publish: Topic %s, Qos %d, Message %s\n",
    //     mqttObj.publish.topic_name, mqttObj.publish.qos, mqttObj.publish.buffer);

    return rc;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// close() - Graceful shutdown of session
// -----------------------------------------------------------------------------
void CWolfMQTTBase::close()
{
    mqtt_net_disconnect(&m_sd);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// wolfMQTT_state_machine() - This task executes the wolfMQTT state machine
// -----------------------------------------------------------------------------
void CWolfMQTTBase::wolfMQTT_state_machine()
{
    char command;

    while(1)
    {
        // Wait for the main thread to tell us all objects are set up
        read(m_pipe[0], &command, 1);

        // Execute wolfMQTT state machine
        while (1)
        {
            int rc = MqttClient_WaitMessage(&mClient, MQTT_CMD_TIMEOUT_MS);

            // If timed-out
            if (rc == MQTT_CODE_ERROR_TIMEOUT)
            {
                // Send keep-alive ping
                rc = MqttClient_Ping(&mClient);
                if (rc != MQTT_CODE_SUCCESS)
                    break;
            }
            // If something failed, exit the loop
            else if (rc != MQTT_CODE_SUCCESS)
                break;
        }
    }
}
// -----------------------------------------------------------------------------