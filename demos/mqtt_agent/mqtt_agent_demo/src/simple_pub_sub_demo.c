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
 * This file demonstrates numerous tasks all of which use the MQTT agent API
 * to send unique MQTT payloads to unique topics over the same MQTT connection
 * to the same MQTT agent.  Some tasks use QoS0 and others QoS1.
 *
 * Each created task is a unique instance of the task implemented by
 * simpleSubscribePublishTask().  simpleSubscribePublishTask()
 * subscribes to a topic then periodically publishes a message to the same
 * topic to which it has subscribed.  The command context sent to
 * MQTTAgent_Publish() contains a unique number that is sent back to the task
 * as a task notification from the callback function that executes when the
 * PUBLISH operation is acknowledged (or just sent in the case of QoS 0).  The
 * task checks the number it receives from the callback equals the number it
 * previously set in the command context before printing out either a success
 * or failure message.
 */

/* Kernel include. */
#include <zephyr.h>

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Demo Specific configs. */
#include "demo_config.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"

/* Subscription manager header include. */
#include "subscription_manager.h"

/**
 * @brief This demo uses task notifications to signal tasks from MQTT callback
 * functions.  MS_TO_WAIT_FOR_NOTIFICATION defines the time, in ticks,
 * to wait for such a callback.
 */
#define MS_TO_WAIT_FOR_NOTIFICATION            ( 10000 )

/**
 * @brief Size of statically allocated buffers for holding topic names and
 * payloads.
 */
#define STRING_BUFFER_LENGTH                   ( 100 )

/**
 * @brief Delay for each task between publishes.
 */
#define DELAY_BETWEEN_PUBLISH_OPERATIONS_MS    ( 1000U )

/**
 * @brief Number of publishes done by each task in this demo.
 */
#define PUBLISH_COUNT                          ( 10UL )

/**
 * @brief The maximum amount of time in milliseconds to wait for the commands
 * to be posted to the MQTT agent should the MQTT agent's command queue be full.
 * Tasks wait in the Blocked state, so don't use any CPU time.
 */
#define MAX_COMMAND_SEND_BLOCK_TIME_MS         ( 500 )

/**
 * @brief The modulus with which to reduce a task number to obtain the task's
 * publish QoS value. Must be either to 1, 2, or 3, resulting in maximum QoS
 * values of 0, 1, and 2, respectively.
 */
#define QOS_MODULUS                            ( 2UL )

/*-----------------------------------------------------------*/

/**
 * @brief Defines the structure to use as the command callback context in this
 * demo.
 */
struct MQTTAgentCommandContext
{
    MQTTStatus_t returnStatus;
    uint32_t taskNum;
    uint32_t notificationValue;
    void * pArgs;
};

/**
 * @brief Parameters for this task.
 */
struct SimplePubSubDemoParams
{
    uint32_t taskNumber;
    bool success;
};

/*-----------------------------------------------------------*/

/**
 * @brief Passed into MQTTAgent_Subscribe() as the callback to execute when the
 * broker ACKs the SUBSCRIBE message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Subscribe() to let the task know the
 * SUBSCRIBE operation completed.  It also sets the returnStatus of the
 * structure passed in as the command's context to the value of the
 * returnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pCommandContext Context of the initial command.
 * @param[in] pReturnInfo The result of the command.
 */
static void subscribeCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                      MQTTAgentReturnInfo_t * pReturnInfo );

/**
 * @brief Passed into MQTTAgent_Publish() as the callback to execute when the
 * broker ACKs the PUBLISH message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Publish() to let the task know the
 * PUBLISH operation completed.  It also sets the returnStatus of the
 * structure passed in as the command's context to the value of the
 * returnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pCommandContext Context of the initial command.
 * @param[in] pReturnInfo The result of the command.
 */
static void publishCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                    MQTTAgentReturnInfo_t * pReturnInfo );

/**
 * @brief Called by the task to wait for a notification from a callback function
 * after the task first executes either MQTTAgent_Publish()* or
 * MQTTAgent_Subscribe().
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pCommandContext Context of the initial command.
 *
 * @return true if the task received a notification, otherwise false.
 */
static bool waitForCommandAcknowledgment( uint32_t taskNumber );

/**
 * @brief Passed into MQTTAgent_Subscribe() as the callback to execute when
 * there is an incoming publish on the topic being subscribed to.  Its
 * implementation just logs information about the incoming publish including
 * the publish messages source topic and payload.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pIncomingPublishCallbackContext Context of the initial command.
 * @param[in] pPublishInfo Deserialized publish.
 */
static void incomingPublishCallback( void * pIncomingPublishCallbackContext,
                                     MQTTPublishInfo_t * pPublishInfo );

/**
 * @brief Subscribe to the topic the demo task will also publish to - that
 * results in all outgoing publishes being published back to the task
 * (effectively echoed back).
 *
 * @param[in] QoS The quality of service (QoS) to use.  Can be zero or one
 * for all MQTT brokers.  Can also be QoS2 if supported by the broker.  AWS IoT
 * does not support QoS2.
 * @param[in] pTopicFilter Topic filter to subscribe to.
 * @param[in] taskNumber Identifier for the task performing the subscribe.
 */
static bool subscribeToTopic( MQTTQoS_t QoS,
                              char * pTopicFilter,
                              uint32_t taskNumber );

/**
 * @brief The function that implements the task demonstrated by this file.
 *
 * @param[in] pParameters A pointer to a SimplePubSubDemoParams struct with
 * parameters for this function.
 * @param[in] b Unused parameter to fit Zephyr thread creation. Can just be NULL.
 * @param[in] c Unused parameter to fit Zephyr thread creation. Can just be NULL.
 */
static void simpleSubscribePublishTask( void * pParameters,
                                        void * b,
                                        void * c );

/*-----------------------------------------------------------*/

/**
 * @brief Semaphore for keeping track of tasks finishing.
 * This is given whenever a task finishes.
 */
extern struct k_sem taskFinishedSem;

/**
 * @brief The MQTT agent manages the MQTT contexts.  This set the handle to the
 * context used by this demo.
 */
extern MQTTAgentContext_t globalMqttAgentContext;

/*-----------------------------------------------------------*/

/**
 * @brief k_thread array to hold thread information for each simple sub pub thread.
 */
static struct k_thread simpleSubPubThreads[ NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ];

/**
 * @brief Semaphore to block at certain points of each thread's running to wait for publishes
 * and subscribes to complete.
 */
static struct k_sem subPubSems[ NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ];

/**
 * @brief The buffer to hold the topic filter. The topic is generated at runtime
 * by adding the task names.
 *
 * @note The topic strings must persist until unsubscribed.
 */
static char topicBuf[ NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE ][ STRING_BUFFER_LENGTH ];

/*-----------------------------------------------------------*/

/**
 * @brief Define pubStackArea as the stack array area for all the simple sub pub threads.
 */
K_THREAD_STACK_ARRAY_DEFINE( pubSubStackArea, NUM_SIMPLE_SUB_PUB_TASKS_TO_CREATE, SIMPLE_SUB_PUB_TASK_STACK_SIZE );

/*-----------------------------------------------------------*/

void StartSimpleSubscribePublishTask( uint32_t numberToCreate,
                                      size_t stackSize,
                                      int priority,
                                      struct SimplePubSubDemoParams * pParams )
{
    uint32_t taskNumber;

    /* Each instance of simpleSubscribePublishTask() generates a unique name
     * and topic filter for itself from the number passed in as the task
     * parameter. */
    /* Create a few instances of simpleSubscribePublishTask(). */
    for( taskNumber = 0; taskNumber < numberToCreate; taskNumber++ )
    {
        k_sem_init( &( subPubSems[ taskNumber ] ), 0, 1 );

        pParams[ taskNumber ].taskNumber = taskNumber;
        pParams[ taskNumber ].success = false;
        k_thread_create( &( simpleSubPubThreads[ taskNumber ] ),
                         pubSubStackArea[ taskNumber ],
                         stackSize,
                         simpleSubscribePublishTask,
                         ( void * ) &pParams[ taskNumber ],
                         NULL,
                         NULL,
                         priority,
                         0,
                         K_NO_WAIT );
    }
}

/*-----------------------------------------------------------*/

static void publishCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                    MQTTAgentReturnInfo_t * pReturnInfo )
{
    /* Store the result in the application defined context so the task that
     * initiated the publish can check the operation's status. */
    pCommandContext->returnStatus = pReturnInfo->returnCode;

    k_sem_give( &( subPubSems[ pCommandContext->taskNum ] ) );
}

/*-----------------------------------------------------------*/

static void subscribeCommandCallback( MQTTAgentCommandContext_t * pCommandContext,
                                      MQTTAgentReturnInfo_t * pReturnInfo )
{
    bool subscriptionAdded = false;
    MQTTAgentSubscribeArgs_t * pxSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pCommandContext->pArgs;

    /* Store the result in the application defined context so the task that
     * initiated the subscribe can check the operation's status. */
    pCommandContext->returnStatus = pReturnInfo->returnCode;

    /* Check if the subscribe operation is a success. Only one topic is
     * subscribed by this demo. */
    if( pReturnInfo->returnCode == MQTTSuccess )
    {
        /* Add subscription so that incoming publishes are routed to the application
         * callback. */
        subscriptionAdded = addSubscription( ( SubscriptionElement_t * ) globalMqttAgentContext.pIncomingCallbackContext,
                                             pxSubscribeArgs->pSubscribeInfo->pTopicFilter,
                                             pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                                             incomingPublishCallback,
                                             NULL );

        if( !subscriptionAdded )
        {
            LogError( ( "Failed to register an incoming publish callback for topic %.*s.",
                        pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                        pxSubscribeArgs->pSubscribeInfo->pTopicFilter ) );
        }
    }

    k_sem_give( &( subPubSems[ pCommandContext->taskNum ] ) );
}

/*-----------------------------------------------------------*/

static bool waitForCommandAcknowledgment( uint32_t taskNumber )
{
    bool returnStatus;

    returnStatus = ( 0 == k_sem_take( &( subPubSems[ taskNumber ] ), K_MSEC( MS_TO_WAIT_FOR_NOTIFICATION ) ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void incomingPublishCallback( void * pIncomingPublishCallbackContext,
                                     MQTTPublishInfo_t * pPublishInfo )
{
    static char terminatedString[ STRING_BUFFER_LENGTH ];

    ( void ) pIncomingPublishCallbackContext;

    /* Create a message that contains the incoming MQTT payload to the logger,
     * terminating the string first. */
    if( pPublishInfo->payloadLength < STRING_BUFFER_LENGTH )
    {
        memcpy( ( void * ) terminatedString, pPublishInfo->pPayload, pPublishInfo->payloadLength );
        terminatedString[ pPublishInfo->payloadLength ] = 0x00;
    }
    else
    {
        memcpy( ( void * ) terminatedString, pPublishInfo->pPayload, STRING_BUFFER_LENGTH );
        terminatedString[ STRING_BUFFER_LENGTH - 1 ] = 0x00;
    }

    LogInfo( ( "Received incoming publish message %s", terminatedString ) );
}

/*-----------------------------------------------------------*/

static bool subscribeToTopic( MQTTQoS_t QoS,
                              char * pTopicFilter,
                              uint32_t taskNumber )
{
    MQTTStatus_t commandAdded;
    bool commandAcknowledged = false;
    uint32_t subscribeMessageID;
    MQTTAgentSubscribeArgs_t subscribeArgs;
    MQTTSubscribeInfo_t subscribeInfo;
    static int32_t nextSubscribeMessageID = 0;
    MQTTAgentCommandContext_t applicationDefinedContext = { 0 };
    MQTTAgentCommandInfo_t commandParams = { 0 };

    nextSubscribeMessageID++;
    subscribeMessageID = nextSubscribeMessageID;

    /* Complete the subscribe information.  The topic string must persist for
     * duration of subscription! */
    subscribeInfo.pTopicFilter = pTopicFilter;
    subscribeInfo.topicFilterLength = ( uint16_t ) strlen( pTopicFilter );
    subscribeInfo.qos = QoS;
    subscribeArgs.pSubscribeInfo = &subscribeInfo;
    subscribeArgs.numSubscriptions = 1;

    /* Complete an application defined context associated with this subscribe message.
     * This gets updated in the callback function so the variable must persist until
     * the callback executes. */
    applicationDefinedContext.notificationValue = subscribeMessageID;
    applicationDefinedContext.taskNum = taskNumber;
    applicationDefinedContext.pArgs = ( void * ) &subscribeArgs;

    commandParams.blockTimeMs = MAX_COMMAND_SEND_BLOCK_TIME_MS;
    commandParams.cmdCompleteCallback = subscribeCommandCallback;
    commandParams.pCmdCompleteCallbackContext = ( void * ) &applicationDefinedContext;

    /* The queue will not become full if the priority of the MQTT agent task is
     * higher than the priority of the task calling this function. */
    LogInfo( ( "Sending subscribe request to agent for topic filter: %s with id %d",
               pTopicFilter,
               ( int ) subscribeMessageID ) );

    commandAdded = MQTTAgent_Subscribe( &globalMqttAgentContext,
                                        &subscribeArgs,
                                        &commandParams );

    if( commandAdded == MQTTSuccess )
    {
        /* Wait for acks to the subscribe message - this is optional but done here
         * so the code below can check the notification sent by the callback matches
         * the nextSubscribeMessageID value set in the context above. */
        commandAcknowledged = waitForCommandAcknowledgment( taskNumber );
    }
    else
    {
        LogError( ( "Failed to enqueue subscribe command." ) );
    }

    /* Check all ways the status was passed back just for demonstration
     * purposes. */
    if( ( commandAcknowledged != true ) ||
        ( applicationDefinedContext.returnStatus != MQTTSuccess ) )
    {
        LogWarn( ( "Error or timed out waiting for ack to subscribe message topic %s",
                   pTopicFilter ) );
    }
    else
    {
        LogInfo( ( "Received subscribe ack for topic %s containing ID %d",
                   pTopicFilter,
                   ( int ) applicationDefinedContext.notificationValue ) );
    }

    return commandAcknowledged;
}

/*-----------------------------------------------------------*/

static void simpleSubscribePublishTask( void * pParameters,
                                        void * b,
                                        void * c )
{
    MQTTPublishInfo_t publishInfo = { 0 };
    char payloadBuf[ STRING_BUFFER_LENGTH ];
    char taskName[ STRING_BUFFER_LENGTH ];
    MQTTAgentCommandContext_t commandContext;
    uint32_t valueToNotify = 0UL;
    MQTTStatus_t commandAdded;
    struct SimplePubSubDemoParams * pParams = ( struct SimplePubSubDemoParams * ) pParameters;
    uint32_t taskNumber = pParams->taskNumber;
    MQTTQoS_t QoS;
    MQTTAgentCommandInfo_t commandParams = { 0 };
    char * pTopicBuffer = topicBuf[ taskNumber ];
    uint32_t numSuccesses = 0U;

    /* Have different tasks use different QoS.  0 and 1.  2 can also be used
     * if supported by the broker. */
    QoS = ( MQTTQoS_t ) ( taskNumber % QOS_MODULUS );

    /* Create a unique name for this task from the task number that is passed into
     * the task using the task's parameter. */
    snprintf( taskName, STRING_BUFFER_LENGTH, "Publisher%d", ( int ) taskNumber );

    /* Create a topic name for this task to publish to. */
    snprintf( pTopicBuffer, STRING_BUFFER_LENGTH, "/filter/%s", taskName );

    /* Subscribe to the same topic to which this task will publish.  That will
     * result in each published message being published from the server back to
     * the target. */
    subscribeToTopic( QoS, pTopicBuffer, taskNumber );

    /* Configure the publish operation. */
    memset( ( void * ) &publishInfo, 0x00, sizeof( publishInfo ) );
    publishInfo.qos = QoS;
    publishInfo.pTopicName = pTopicBuffer;
    publishInfo.topicNameLength = ( uint16_t ) strlen( pTopicBuffer );
    publishInfo.pPayload = payloadBuf;

    /* Store the handler to this task in the command context so the callback
     * that executes when the command is acknowledged can send a notification
     * back to this task. */
    memset( ( void * ) &commandContext, 0x00, sizeof( commandContext ) );
    commandContext.taskNum = taskNumber;

    commandParams.blockTimeMs = MAX_COMMAND_SEND_BLOCK_TIME_MS;
    commandParams.cmdCompleteCallback = publishCommandCallback;
    commandParams.pCmdCompleteCallbackContext = &commandContext;

    /* For a finite number of publishes... */
    for( valueToNotify = 0UL; valueToNotify < PUBLISH_COUNT; valueToNotify++ )
    {
        /* Create a payload to send with the publish message.  This contains
         * the task name and an incrementing number. */
        snprintf( payloadBuf,
                  STRING_BUFFER_LENGTH,
                  "%s publishing message %d",
                  taskName,
                  ( int ) valueToNotify );

        publishInfo.payloadLength = ( uint16_t ) strlen( payloadBuf );

        /* Also store the incrementing number in the command context so it can
         * be accessed by the callback that executes when the publish operation
         * is acknowledged. */
        commandContext.notificationValue = valueToNotify;

        LogInfo( ( "Sending publish request to agent with message \"%s\" on topic \"%s\"",
                   payloadBuf,
                   pTopicBuffer ) );

        commandAdded = MQTTAgent_Publish( &globalMqttAgentContext,
                                          &publishInfo,
                                          &commandParams );

        if( commandAdded == MQTTSuccess )
        {
            /* For QoS 1 and 2, wait for the publish acknowledgment.  For QoS0,
             * wait for the publish to be sent. */
            LogInfo( ( "Task %s waiting for publish %d to complete.",
                       taskName,
                       valueToNotify ) );
            waitForCommandAcknowledgment( taskNumber );
        }
        else
        {
            LogError( ( "Failed to enqueue publish command. Error code=%s", MQTT_Status_strerror( commandAdded ) ) );
        }

        /* The value received by the callback that executed when the publish was
         * completed came from the context passed into MQTTAgent_Publish() above,
         * so should match the value set in the context above. */
        numSuccesses++;
        /* Log statement to indicate successful reception of publish. */
        LogInfo( ( "Publish %d completed successfully.\r\n", valueToNotify ) );

        LogInfo( ( "Short delay before next publish... \r\n\r\n" ) );

        k_sleep( K_MSEC( DELAY_BETWEEN_PUBLISH_OPERATIONS_MS ) );
    }

    /* Mark this task as successful if every publish was successfully completed. */
    if( numSuccesses == PUBLISH_COUNT )
    {
        pParams->success = true;
        LogInfo( ( "Task %s successful.", taskName ) );
    }

    k_sem_give( &taskFinishedSem );

    /* Task will terminate itself after returning from entry (this) function. */
}
