/*
 * AWS IoT Device Embedded C SDK for ZephyrRTOS
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This demo creates multiple tasks, all of which use the MQTT agent API to
 * communicate with an MQTT broker through the same MQTT connection.
 *
 * This file contains the initial task created after the TCP/IP stack connects
 * to the network.  The task:
 *
 * 1) Connects to the MQTT broker.
 * 2) Creates the other demo tasks, in accordance with the #defines set in
 *    demo_config.h.  For example, if demo_config.h contains the following
 *    setting:
 *
 *    #define NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE 3
 *
 *    then the initial task will create three instances of the task
 *    implemented in simple_sub_pub_demo.c.  See the comments at the top
 *    of that file for more information.
 *
 * 3) After creating the demo tasks the initial task will create the MQTT
 *    agent task.
 */

/* Standard includes. */
#include <string.h>

/* Kernel includes. */
#include <zephyr.h>

/* Zephyr includes */
#include <random/rand32.h>
#include <net/socket.h>

/* Demo Specific configs. */
#include "demo_config.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"

/* MQTT Agent ports. */
#include "agent_interface_zephyr.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Subscription manager header include. */
#include "subscription_manager.h"

/* Transport interface implementation include header for TLS. */
#include "mbedtls_zephyr.h"

/* Wifi connection for ESP32 */
#include "esp_wifi_wrapper.h"

/**
 * These configuration settings are required to run the demo.
 * Throw compilation error if the below configs are not defined.
 */
#ifndef MQTT_BROKER_ENDPOINT
    #error "Please define AWS IoT MQTT broker endpoint(AWS_IOT_ENDPOINT) in demo_config.h."
#endif
#ifndef ROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(ROOT_CA_CERT_PEM) in demo_config.h."
#endif
#ifndef CLIENT_IDENTIFIER
    #error "Please define a unique client identifier, CLIENT_IDENTIFIER, in demo_config.h."
#endif
#ifndef WIFI_NETWORK_SSID
    #error "Please define the wifi network ssid, in demo_config.h."
#endif
#ifndef WIFI_NETWORK_PASSWORD
    #error "Please define the wifi network's password in demo_config.h."
#endif

/* This demo uses compile time options to select the demo tasks to created.
 * Ensure the compile time options are defined.  These should be defined in
 * demo_config.h. */
#if !defined( NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ) || ( NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE < 1 )
    #error Please set NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE to the number of tasks to create in vStartSimpleSubscribePublishTask().
#endif

#if ( NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE > 0 ) && !defined( SIMPLE_SUB_PUB_TASK_STACK_SIZE )
    #error Please define SIMPLE_SUB_PUB_TASK_STACK_SIZE in demo_config.h to set the stack size (in words, not bytes) for the tasks created by vStartSimpleSubscribePublishTask().
#endif

/**
 * @brief Dimensions the buffer used to serialize and deserialize MQTT packets.
 * @note Specified in bytes.  Must be large enough to hold the maximum
 * anticipated MQTT payload.
 */
#ifndef MQTT_AGENT_NETWORK_BUFFER_SIZE
    #define MQTT_AGENT_NETWORK_BUFFER_SIZE    ( 5000 )
#endif

/**
 * @brief The length of the queue used to hold commands for the agent.
 */
#ifndef MQTT_AGENT_COMMAND_QUEUE_LENGTH
    #define MQTT_AGENT_COMMAND_QUEUE_LENGTH    ( 10 )
#endif

/**
 * @brief Length of client identifier.
 */
#define CLIENT_IDENTIFIER_LENGTH    ( ( uint16_t ) ( sizeof( CLIENT_IDENTIFIER ) - 1 ) )

/**
 * @brief Length of MQTT server host name.
 */
#define BROKER_ENDPOINT_LENGTH      ( ( uint16_t ) ( sizeof( MQTT_BROKER_ENDPOINT ) - 1 ) )

/**
 * These configuration settings are required to run the demo.
 */

/**
 * @brief Timeout for receiving CONNACK after sending an MQTT CONNECT packet.
 * Defined in milliseconds.
 */
#define CONNACK_RECV_TIMEOUT_MS           ( 1000U )

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define RETRY_MAX_ATTEMPTS                ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define RETRY_MAX_BACKOFF_DELAY_MS        ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define RETRY_BACKOFF_BASE_MS             ( 500U )

/**
 * @brief The maximum time interval in seconds which is allowed to elapse
 *  between two Control Packets.
 *
 *  It is the responsibility of the Client to ensure that the interval between
 *  Control Packets being sent does not exceed the this Keep Alive value. In the
 *  absence of sending any other Control Packets, the Client MUST send a
 *  PINGREQ Packet.
 *//*_RB_ Move to be the responsibility of the agent. */
#define KEEP_ALIVE_INTERVAL_SECONDS       ( 60U )

/**
 * @brief Socket send and receive timeouts to use.  Specified in milliseconds.
 */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 750 )

/*-----------------------------------------------------------*/

/**
 * @brief Each compilation unit that consumes the NetworkContext must define it.
 * It should contain a single pointer to the type of your desired transport.
 * When using multiple transports in the same compilation unit, define this pointer as void *.
 *
 * @note Transport stacks are defined in amazon-freertos/libraries/abstractions/transport/secure_sockets/transport_secure_sockets.h.
 */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

/**
 * @brief Parameters for subscribe-publish tasks.
 */
struct MQTTAgentDemoParams
{
    uint32_t taskNumber;
    bool success;
};

/*-----------------------------------------------------------*/

/**
 * @brief Create the task that demonstrates the sharing of an MQTT connection
 * using the coreMQTT Agent library.
 *
 * @return EXIT_SUCCESS if demo completes successfully, else EXIT_FAILURE.
 */
static int runCoreMqttAgentDemo( bool awsIotMqttMode,
                                 const char * pIdentifier,
                                 void * pNetworkServerInfo,
                                 void * pNetworkCredentialInfo,
                                 const void * pNetworkInterface );

/**
 * @brief Initializes an MQTT Agent context, including transport interface,
 * network buffer, and publish callback.
 *
 * @return `MQTTSuccess` if the initialization succeeds, else `MQTTBadParameter`.
 */
static MQTTStatus_t mqttAgentInit( void );

/**
 * @brief Sends an MQTT Connect packet over the already connected TCP socket.
 *
 * @param[in] cleanSession If a clean session should be established.
 *
 * @return `MQTTSuccess` if connection succeeds, else appropriate error code
 * from MQTT_Connect.
 */
static MQTTStatus_t mqttConnect( bool cleanSession );

/**
 * @brief Calculate and perform an exponential backoff with jitter delay for
 * the next retry attempt of a failed network operation with the server.
 *
 * The function generates a random number, calculates the next backoff period
 * with the generated random number, and performs the backoff delay operation if the
 * number of retries have not exhausted.
 *
 * @note The backoff period is calculated using the backoffAlgorithm library.
 *
 * @param[in, out] pRetryParams The context to use for backoff period calculation
 * with the backoffAlgorithm library.
 *
 * @return true if calculating the backoff period was successful; otherwise false
 * if there was failure in random number generation OR all retry attempts had exhausted.
 */
static bool backoffForRetry( BackoffAlgorithmContext_t * pRetryParams );

/**
 * @brief Connect a TCP socket to the MQTT broker.
 *
 * @param[in] pNetworkContext Network context.
 *
 * @return true if connection succeeds, else false.
 */
static bool socketConnect( NetworkContext_t * pNetworkContext );

/**
 * @brief Disconnect a TCP connection.
 *
 * @param[in] pNetworkContext Network context.
 *
 * @return true if disconnect succeeds, else false.
 */
static bool socketDisconnect( NetworkContext_t * pNetworkContext );

/**
 * @brief Fan out the incoming publishes to the callbacks registered by different
 * tasks. If there are no callbacks registered for the incoming publish, it will be
 * passed to the unsolicited publish handler.
 *
 * @param[in] pMqttAgentContext Agent context.
 * @param[in] packetId Packet ID of publish.
 * @param[in] pPublishInfo Info of incoming publish.
 */
static void incomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
                                     uint16_t packetId,
                                     MQTTPublishInfo_t * pPublishInfo );

/**
 * @brief Function to attempt to resubscribe to the topics already present in the
 * subscription list.
 *
 * This function will be invoked when this demo requests the broker to
 * reestablish the session and the broker cannot do so. This function will
 * enqueue commands to the MQTT Agent queue and will be processed once the
 * command loop starts.
 *
 * @return `MQTTSuccess` if adding subscribes to the command queue succeeds, else
 * appropriate error code from MQTTAgent_Subscribe.
 * */
static MQTTStatus_t handleResubscribe( void );

/**
 * @brief Passed into MQTTAgent_Subscribe() as the callback to execute when the
 * broker ACKs the SUBSCRIBE message. This callback implementation is used for
 * handling the completion of resubscribes. Any topic filter failed to resubscribe
 * will be removed from the subscription list.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pCommandContext Context of the initial command.
 * @param[in] pReturnInfo The result of the command.
 */
static void subscriptionCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                         MQTTAgentReturnInfo_t * pReturnInfo );

/**
 * @brief Task used to run the MQTT agent.  In this example the first task that
 * is created is responsible for creating all the other demo tasks.  Then,
 * rather than create mqttAgentTask() as a separate task, it simply calls
 * mqttAgentTask() to become the agent task itself.
 *
 * This task calls MQTTAgent_CommandLoop() in a loop, until MQTTAgent_Terminate()
 * is called. If an error occurs in the command loop, then it will reconnect the
 * TCP and MQTT connections.
 *
 * @param[in] pParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 * @param[in] b Unused parameter to fit Zephyr thread creation. Can just be NULL.
 * @param[in] c Unused parameter to fit Zephyr thread creation. Can just be NULL.
 */
static void mqttAgentTask( void * pParameters,
                           void * b,
                           void * c );

/**
 * @brief The main task used in the MQTT demo.
 *
 * This task creates the network connection and all other demo tasks. Then,
 * rather than create mqttAgentTask() as a separate task, it simply calls
 * mqttAgentTask() to become the agent task itself.
 *
 * @param[in] pParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 *
 * @return EXIT_SUCCESS if demo completes successfully, else EXIT_FAILURE.
 */
static int connectAndCreateDemoTasks( void * pParameters );

/**
 * @brief The timer query function provided to the MQTT context.
 *
 * @return Time in milliseconds.
 */
static uint32_t getTimeMs( void );

/**
 * @brief Connects a TCP socket to the MQTT broker, then creates and MQTT
 * connection to the same.
 *
 * @param[in] createCleanSession Whether to create a clean session.
 */
static bool connectToMQTTBroker( bool createCleanSession );

/*
 * Function that starts the tasks demonstrated by this project.
 */
extern void StartSimpleSubscribePublishTask( uint32_t taskNumber,
                                             size_t stackSize,
                                             int priority,
                                             struct MQTTAgentDemoParams * pParams );

/*-----------------------------------------------------------*/

/**
 * @brief The network context used by the MQTT library transport interface.
 * See https://www.freertos.org/network-interface.html
 */
static NetworkContext_t networkContext;

/**
 * @brief The parameters for the network context using a TLS channel.
 */
static TlsTransportParams_t secureSocketsTransportParams;

/**
 * @brief Global entry time into the application to use as a reference timestamp
 * in the #getTimeMs function. #getTimeMs will always return the difference
 * between the current time and the global entry time. This will reduce the chances
 * of overflow for the 32 bit unsigned integer used for holding the timestamp.
 */
static uint32_t globalEntryTimeMs;

/**
 * @brief Global MQTT Agent context used by every task.
 */
MQTTAgentContext_t globalMqttAgentContext;

/**
 * @brief Network buffer for coreMQTT.
 */
static uint8_t networkBuffer[ MQTT_AGENT_NETWORK_BUFFER_SIZE ];

/**
 * @brief Message queue used to deliver commands to the agent task.
 */
static MQTTAgentMessageContext_t commandQueue;

/**
 * @brief Structs to hold input and output parameters for each subscribe-publish task.
 */
static struct MQTTAgentDemoParams taskParameters[ NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ];

/**
 * @brief The global array of subscription elements.
 *
 * @note No thread safety is required to this array, since updates to the array
 * elements are done only from the MQTT agent task. The subscription manager
 * implementation expects that the array of the subscription elements used for
 * storing subscriptions to be initialized to 0. As this is a global array, it
 * will be initialized to 0 by default.
 */
SubscriptionElement_t globalSubscriptionList[ SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS ];

/**
 * @brief Semaphore to block until all tasks are finished.
 */
struct k_sem taskFinishedSem;

/**
 * @brief k_thread struct to hold MQTT Agent thread information.
 */
static struct k_thread mqttAgentThread;

/**
 * @brief The command queue is a Zephyr message queue, which needs a buffer to store its members.
 */
char __aligned( 8 ) commandQueueBuffer[ MQTT_AGENT_COMMAND_QUEUE_LENGTH * sizeof( MQTTAgentCommand_t * ) ];

/*-----------------------------------------------------------*/

/**
 * @brief Define mqttAgentStackArea as the stack area for the MQTT Agent thread.
 */
K_THREAD_STACK_DEFINE( mqttAgentStackArea, SIMPLE_SUB_PUB_TASK_STACK_SIZE );

/*-----------------------------------------------------------*/

static int runCoreMqttAgentDemo( bool awsIotMqttMode,
                                 const char * pIdentifier,
                                 void * pNetworkServerInfo,
                                 void * pNetworkCredentialInfo,
                                 const void * pNetworkInterface )
{
    uint32_t demoCount = 0UL;
    int returnStatus = EXIT_SUCCESS;

    ( void ) awsIotMqttMode;
    ( void ) pIdentifier;
    ( void ) pNetworkServerInfo;
    ( void ) pNetworkCredentialInfo;
    ( void ) pNetworkInterface;

    for( demoCount = 0UL; ( demoCount < MQTT_MAX_DEMO_COUNT ); demoCount++ )
    {
        returnStatus = connectAndCreateDemoTasks( NULL );

        if( returnStatus == EXIT_SUCCESS )
        {
            LogInfo( ( "Demo iteration %d successful.", ( demoCount + 1 ) ) );
            break;
        }
        else if( demoCount < ( MQTT_MAX_DEMO_COUNT - 1 ) )
        {
            LogWarn( ( "Demo iteration %d failed. Retrying...", ( demoCount + 1 ) ) );
        }
        else
        {
            LogError( ( "All %d iterations failed", MQTT_MAX_DEMO_COUNT ) );
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t mqttAgentInit( void )
{
    TransportInterface_t transport;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t fixedBuffer = { .pBuffer = networkBuffer, .size = MQTT_AGENT_NETWORK_BUFFER_SIZE };
    MQTTAgentMessageInterface_t messageInterface =
    {
        .pMsgCtx        = NULL,
        .send           = Agent_MessageSend,
        .recv           = Agent_MessageReceive,
        .getCommand     = Agent_GetCommand,
        .releaseCommand = Agent_FreeCommand
    };

    LogDebug( ( "Creating command queue." ) );
    k_msgq_init( &( commandQueue.queue ), commandQueueBuffer, sizeof( MQTTAgentCommand_t * ), MQTT_AGENT_COMMAND_QUEUE_LENGTH );
    messageInterface.pMsgCtx = &commandQueue;

    Agent_InitializePool();

    /* Fill in Transport Interface send and receive function pointers. */
    transport.pNetworkContext = &networkContext;
    transport.send = MbedTLS_send;
    transport.recv = MbedTLS_recv;

    /* Initialize MQTT library. */
    mqttStatus = MQTTAgent_Init( &globalMqttAgentContext,
                                 &messageInterface,
                                 &fixedBuffer,
                                 &transport,
                                 getTimeMs,
                                 incomingPublishCallback,
                                 /* Context to pass into the callback. Passing the pointer to subscription array. */
                                 globalSubscriptionList );

    return mqttStatus;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t mqttConnect( bool cleanSession )
{
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo;
    bool sessionPresent = false;

    /* Many fields are not used in this demo so start with everything at 0. */
    memset( &connectInfo, 0x00, sizeof( connectInfo ) );

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    connectInfo.cleanSession = cleanSession;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    connectInfo.pClientIdentifier = CLIENT_IDENTIFIER;
    connectInfo.clientIdentifierLength = ( uint16_t ) strlen( CLIENT_IDENTIFIER );

    /* Set MQTT keep-alive period. It is the responsibility of the application
     * to ensure that the interval between Control Packets being sent does not
     * exceed the Keep Alive value. In the absence of sending any other Control
     * Packets, the Client MUST send a PINGREQ Packet.  This responsibility will
     * be moved inside the agent. */
    connectInfo.keepAliveSeconds = KEEP_ALIVE_INTERVAL_SECONDS;

    /* Append metrics when connecting to the AWS IoT Core broker. */
    #ifdef USE_AWS_IOT_CORE_BROKER
        #ifdef CLIENT_USERNAME
            connectInfo.pUserName = CLIENT_USERNAME_WITH_METRICS;
            connectInfo.userNameLength = ( uint16_t ) strlen( CLIENT_USERNAME_WITH_METRICS );
            connectInfo.pPassword = CLIENT_PASSWORD;
            connectInfo.passwordLength = ( uint16_t ) strlen( CLIENT_PASSWORD );
        #else
            connectInfo.pUserName = AWS_IOT_METRICS_STRING;
            connectInfo.userNameLength = AWS_IOT_METRICS_STRING_LENGTH;
            /* Password for authentication is not used. */
            connectInfo.pPassword = NULL;
            connectInfo.passwordLength = 0U;
        #endif
    #else /* ifdef USE_AWS_IOT_CORE_BROKER */
        #ifdef CLIENT_USERNAME
            connectInfo.pUserName = CLIENT_USERNAME;
            connectInfo.userNameLength = ( uint16_t ) strlen( CLIENT_USERNAME );
            connectInfo.pPassword = CLIENT_PASSWORD;
            connectInfo.passwordLength = ( uint16_t ) strlen( CLIENT_PASSWORD );
        #endif /* ifdef CLIENT_USERNAME */
    #endif /* ifdef USE_AWS_IOT_CORE_BROKER */

    /* Send MQTT CONNECT packet to broker. MQTT's Last Will and Testament feature
     * is not used in this demo, so it is passed as NULL. */
    mqttStatus = MQTT_Connect( &( globalMqttAgentContext.mqttContext ),
                               &connectInfo,
                               NULL,
                               CONNACK_RECV_TIMEOUT_MS,
                               &sessionPresent );

    LogInfo( ( "Session present: %d\n", sessionPresent ) );

    /* Resume a session if desired. */
    if( ( mqttStatus == MQTTSuccess ) && ( !cleanSession ) )
    {
        mqttStatus = MQTTAgent_ResumeSession( &globalMqttAgentContext, sessionPresent );

        /* Resubscribe to all the subscribed topics. */
        if( ( mqttStatus == MQTTSuccess ) && ( !sessionPresent ) )
        {
            mqttStatus = handleResubscribe();
        }
    }

    return mqttStatus;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t handleResubscribe( void )
{
    MQTTStatus_t mqttStatus = MQTTBadParameter;
    uint32_t index = 0U;
    uint16_t numSubscriptions = 0U;

    /* These variables need to stay in scope until command completes. */
    static MQTTAgentSubscribeArgs_t subArgs = { 0 };
    static MQTTSubscribeInfo_t subInfo[ SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS ] = { 0 };
    static MQTTAgentCommandInfo_t commandParams = { 0 };

    /* Loop through each subscription in the subscription list and add a subscribe
     * command to the command queue. */
    for( index = 0U; index < SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS; index++ )
    {
        /* Check if there is a subscription in the subscription list. This demo
         * doesn't check for duplicate subscriptions. */
        if( globalSubscriptionList[ index ].filterStringLength != 0 )
        {
            subInfo[ numSubscriptions ].pTopicFilter = globalSubscriptionList[ index ].pSubscriptionFilterString;
            subInfo[ numSubscriptions ].topicFilterLength = globalSubscriptionList[ index ].filterStringLength;

            /* QoS1 is used for all the subscriptions in this demo. */
            subInfo[ numSubscriptions ].qos = MQTTQoS1;

            LogInfo( ( "Resubscribe to the topic %.*s will be attempted.",
                       subInfo[ numSubscriptions ].topicFilterLength,
                       subInfo[ numSubscriptions ].pTopicFilter ) );

            numSubscriptions++;
        }
    }

    if( numSubscriptions > 0U )
    {
        subArgs.pSubscribeInfo = subInfo;
        subArgs.numSubscriptions = numSubscriptions;

        /* The block time can be 0 as the command loop is not running at this point. */
        commandParams.blockTimeMs = 0U;
        commandParams.cmdCompleteCallback = subscriptionCommandCallback;
        commandParams.pCmdCompleteCallbackContext = ( void * ) &subArgs;

        /* Enqueue subscribe to the command queue. These commands will be processed only
         * when command loop starts. */
        mqttStatus = MQTTAgent_Subscribe( &globalMqttAgentContext, &subArgs, &commandParams );
    }
    else
    {
        /* Mark the resubscribe as success if there is nothing to be subscribed. */
        mqttStatus = MQTTSuccess;
    }

    if( mqttStatus != MQTTSuccess )
    {
        LogError( ( "Failed to enqueue the MQTT subscribe command. mqttStatus=%s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
    }

    return mqttStatus;
}

/*-----------------------------------------------------------*/

static void subscriptionCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                         MQTTAgentReturnInfo_t * pReturnInfo )
{
    size_t index = 0;
    MQTTAgentSubscribeArgs_t * pSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pCommandContext;

    /* If the return code is success, no further action is required as all the topic filters
     * are already part of the subscription list. */
    if( pReturnInfo->returnCode != MQTTSuccess )
    {
        /* Check through each of the suback codes and determine if there are any failures. */
        for( index = 0; index < pSubscribeArgs->numSubscriptions; index++ )
        {
            /* This demo doesn't attempt to resubscribe in the event that a SUBACK failed. */
            if( pReturnInfo->pSubackCodes[ index ] == MQTTSubAckFailure )
            {
                LogError( ( "Failed to resubscribe to topic %.*s.",
                            pSubscribeArgs->pSubscribeInfo[ index ].topicFilterLength,
                            pSubscribeArgs->pSubscribeInfo[ index ].pTopicFilter ) );
                /* Remove subscription callback for unsubscribe. */
                removeSubscription( globalSubscriptionList,
                                    pSubscribeArgs->pSubscribeInfo[ index ].pTopicFilter,
                                    pSubscribeArgs->pSubscribeInfo[ index ].topicFilterLength );
            }
        }
    }
}

/*-----------------------------------------------------------*/

static bool backoffForRetry( BackoffAlgorithmContext_t * pRetryParams )
{
    bool returnStatus = false;
    uint16_t nextRetryBackOff = 0U;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;

    /**
     * To calculate the backoff period for the next retry attempt, we will
     * generate a random number to provide to the backoffAlgorithm library.
     */
    uint32_t randomNum = sys_rand32_get();

    /* Get back-off value (in milliseconds) for the next retry attempt. */
    backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( pRetryParams, randomNum, &nextRetryBackOff );

    if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
    {
        LogError( ( "All retry attempts have exhausted. Operation will not be retried" ) );
    }
    else if( backoffAlgStatus == BackoffAlgorithmSuccess )
    {
        /* Perform the backoff delay. */
        k_sleep( K_MSEC( nextRetryBackOff ) );

        returnStatus = true;

        LogInfo( ( "Retry attempt %d out of maximum retry attempts %d.",
                   ( pRetryParams->attemptsDone + 1 ),
                   pRetryParams->maxRetryAttempts ) );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static bool socketConnect( NetworkContext_t * pNetworkContext )
{
    bool connected = false;
    TlsTransportStatus_t networkStatus = TLS_TRANSPORT_SUCCESS;
    ServerInfo_t serverInfo = { 0 };
    NetworkCredentials_t networkCredentials = { 0 };
    /* Set the receive timeout to a small nonzero value. */
    const uint32_t transportTimeout = 1U;

    /* Initialize the MQTT broker information. */
    serverInfo.pHostName = MQTT_BROKER_ENDPOINT;
    serverInfo.hostNameLength = sizeof( MQTT_BROKER_ENDPOINT ) - 1U;
    serverInfo.port = MQTT_BROKER_PORT;

    /* Set the Secure Socket configurations. */
    networkCredentials.disableSni = DISABLE_SNI;
    networkCredentials.pRootCa = ROOT_CA_PEM;
    networkCredentials.rootCaSize = sizeof( ROOT_CA_PEM );

    /* Establish a TCP connection with the MQTT broker. This example connects to
     * the MQTT broker as specified in MQTT_BROKER_ENDPOINT and
     * MQTT_BROKER_PORT at the top of this file. */
    LogInfo( ( "Creating a TLS connection to %s:%d.",
               MQTT_BROKER_ENDPOINT,
               MQTT_BROKER_PORT ) );

    networkStatus = MbedTLS_Connect( pNetworkContext, &serverInfo, &networkCredentials, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS );

    connected = ( networkStatus == TLS_TRANSPORT_SUCCESS );

    /* Set the socket wakeup callback and ensure the read block time. */
    if( connected )
    {
        zsock_setsockopt( pNetworkContext->pParams->tcpSocket,
                          0,
                          SO_RCVTIMEO,
                          &( K_TICKS( transportTimeout ) ),
                          sizeof( k_timeout_t ) );
    }

    return connected;
}

/*-----------------------------------------------------------*/

static bool socketDisconnect( NetworkContext_t * pNetworkContext )
{
    LogInfo( ( "Disconnecting TLS connection.\n" ) );
    MbedTLS_Disconnect( pNetworkContext );

    return true;
}

/*-----------------------------------------------------------*/

static void incomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
                                     uint16_t packetId,
                                     MQTTPublishInfo_t * pPublishInfo )
{
    bool publishHandled = false;
    char originalChar, * pLocation;

    ( void ) packetId;

    /* Fan out the incoming publishes to the callbacks registered using
     * subscription manager. */
    publishHandled = handleIncomingPublishes( ( SubscriptionElement_t * ) pMqttAgentContext->pIncomingCallbackContext,
                                              pPublishInfo );

    /* If there are no callbacks to handle the incoming publishes,
     * handle it as an unsolicited publish. */
    if( !publishHandled )
    {
        /* Ensure the topic string is terminated for printing.  This will over-
         * write the message ID, which is restored afterwards. */
        pLocation = ( char * ) &( pPublishInfo->pTopicName[ pPublishInfo->topicNameLength ] );
        originalChar = *pLocation;
        *pLocation = 0x00;
        LogWarn( ( "Received an unsolicited publish from topic %s", pPublishInfo->pTopicName ) );
        *pLocation = originalChar;
    }
}

/*-----------------------------------------------------------*/

static void mqttAgentTask( void * pParameters,
                           void * b,
                           void * c )
{
    bool networkResult = false;
    MQTTStatus_t mqttStatus = MQTTSuccess, xConnectStatus = MQTTSuccess;
    MQTTContext_t * pMqttContext = &( globalMqttAgentContext.mqttContext );

    ( void ) pParameters;
    ( void ) b;
    ( void ) c;

    do
    {
        /* MQTTAgent_CommandLoop() is effectively the agent implementation.  It
         * will manage the MQTT protocol until such time that an error occurs,
         * which could be a disconnect.  If an error occurs the MQTT context on
         * which the error happened is returned so there can be an attempt to
         * clean up and reconnect however the application writer prefers. */
        mqttStatus = MQTTAgent_CommandLoop( &globalMqttAgentContext );

        /* Success is returned for disconnect or termination. The socket should
         * be disconnected. */
        if( ( mqttStatus == MQTTSuccess ) && ( globalMqttAgentContext.mqttContext.connectStatus == MQTTNotConnected ) )
        {
            /* MQTT Disconnect. Disconnect the socket. */
            networkResult = socketDisconnect( &networkContext );
        }
        else if( mqttStatus == MQTTSuccess )
        {
            /* MQTTAgent_Terminate() was called, but MQTT was not disconnected. */
            xConnectStatus = MQTT_Disconnect( &( globalMqttAgentContext.mqttContext ) );
            networkResult = socketDisconnect( &networkContext );
            break;
        }
        /* Any error. */
        else
        {
            /* Reconnect TCP. */
            networkResult = socketDisconnect( &networkContext );

            if( networkResult )
            {
                pMqttContext->connectStatus = MQTTNotConnected;

                /* MQTT Connect with a persistent session. */
                networkResult = connectToMQTTBroker( false );
            }

            if( !networkResult )
            {
                LogError( ( "Could not reconnect to MQTT broker" ) );
                break;
            }
        }
    } while( mqttStatus != MQTTSuccess );

    /* Delete the task if it is complete. Zephyr thread should self terminate. */
    LogInfo( ( "MQTT Agent task completed." ) );
}

/*-----------------------------------------------------------*/

static bool connectToMQTTBroker( bool createCleanSession )
{
    MQTTStatus_t mqttStatus = MQTTBadParameter;
    bool connected = false;
    BackoffAlgorithmContext_t reconnectParams;
    bool backoffStatus = false;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &reconnectParams,
                                       RETRY_BACKOFF_BASE_MS,
                                       RETRY_MAX_BACKOFF_DELAY_MS,
                                       RETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after a
     * timeout. Timeout value will exponentially increase until the maximum
     * number of attempts are reached.
     */
    do
    {
        /* Connect a TCP socket to the broker. */
        connected = socketConnect( &networkContext );

        if( connected )
        {
            /* Form an MQTT connection. */
            mqttStatus = mqttConnect( createCleanSession );

            if( mqttStatus != MQTTSuccess )
            {
                /* Close connection before next retry. */
                socketDisconnect( &networkContext );
            }
        }

        if( mqttStatus != MQTTSuccess )
        {
            LogWarn( ( "Connection to the broker failed. Attempting connection retry after backoff delay." ) );

            /* As the connection attempt failed, we will retry the connection after an
             * exponential backoff with jitter delay. */

            /* Calculate the backoff period for the next retry attempt and perform the wait operation. */
            backoffStatus = backoffForRetry( &reconnectParams );
        }
    } while( ( mqttStatus != MQTTSuccess ) && ( backoffStatus ) );

    return mqttStatus == MQTTSuccess;
}
/*-----------------------------------------------------------*/

static int connectAndCreateDemoTasks( void * pParameters )
{
    MQTTAgentCommandInfo_t commandParams = { 0 };
    uint32_t i, numSuccess = 0;
    bool result = false;
    MQTTStatus_t mqttStatus = MQTTBadParameter;

    ( void ) pParameters;

    /* Miscellaneous initialization. */
    globalEntryTimeMs = getTimeMs();

    /* Set the pParams member of the network context with desired transport. */
    networkContext.pParams = &secureSocketsTransportParams;

    /* Initialize the MQTT context with the buffer and transport interface. */
    mqttStatus = mqttAgentInit();

    if( mqttStatus == MQTTSuccess )
    {
        /* Create the TCP connection to the broker, then the MQTT connection to the
         * same. */
        result = connectToMQTTBroker( true );
    }

    if( result )
    {
        k_sem_init( &taskFinishedSem, 0, NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE );
        /* Create demo tasks as per the configuration macro settings. */
        StartSimpleSubscribePublishTask( NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE,
                                         SIMPLE_SUB_PUB_TASK_STACK_SIZE,
                                         5,
                                         taskParameters );

        /* Create an instance of the MQTT agent task. Give it higher priority than the
         * subscribe-publish tasks so that the agent's command queue will not become full,
         * as those tasks need to send commands to the queue. */
        k_thread_create( &mqttAgentThread,
                         mqttAgentStackArea,
                         SIMPLE_SUB_PUB_TASK_STACK_SIZE,
                         mqttAgentTask,
                         NULL,
                         NULL,
                         NULL,
                         4,
                         0,
                         K_NO_WAIT );

        /* Wait for all tasks to exit. */
        for( i = 0; i < NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE; i++ )
        {
            k_sem_take( &taskFinishedSem, K_FOREVER );
        }

        /* Terminate the agent task. */
        MQTTAgent_Terminate( &globalMqttAgentContext, &commandParams );

        for( i = 0; i < NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE; i++ )
        {
            if( taskParameters[ i ].success )
            {
                numSuccess++;
            }
        }
    }

    LogInfo( ( "%d/%d tasks successful.", numSuccess, NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ) );

    return ( numSuccess == NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*-----------------------------------------------------------*/

static uint32_t getTimeMs( void )
{
    return k_uptime_get_32() - globalEntryTimeMs;
}

/*-----------------------------------------------------------*/

void main()
{
    LogInfo( ( "Connecting to WiFi network: SSID=%.*s ...", strlen( WIFI_NETWORK_SSID ), WIFI_NETWORK_SSID ) );

    if( Wifi_Connect( WIFI_NETWORK_SSID, strlen( WIFI_NETWORK_SSID ), WIFI_NETWORK_PASSWORD, strlen( WIFI_NETWORK_PASSWORD ) ) )
    {
        runCoreMqttAgentDemo( NULL, NULL, NULL, NULL, NULL );
    }
    else
    {
        LogError( ( "Unable to attempt wifi connection. Demo terminating." ) );
    }
}
