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

/**
 * @file subscription_manager.c
 * @brief Functions for managing MQTT subscriptions.
 */

/* Standard includes. */
#include <string.h>

/* Subscription manager header include. */
#include "subscription_manager.h"

/*-----------------------------------------------------------*/

bool addSubscription( SubscriptionElement_t * pSubscriptionList,
                      const char * pTopicFilterString,
                      uint16_t topicFilterLength,
                      IncomingPubCallback_t incomingPublishCallback,
                      void * pIncomingPublishCallbackContext )
{
    int32_t index = 0;
    size_t availableIndex = SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS;
    bool returnStatus = false;

    if( ( pSubscriptionList == NULL ) ||
        ( pTopicFilterString == NULL ) ||
        ( topicFilterLength == 0U ) ||
        ( incomingPublishCallback == NULL ) )
    {
        LogError( ( "Invalid parameter. pSubscriptionList=%p, pTopicFilterString=%p,"
                    " topicFilterLength=%u, incomingPublishCallback=%p.",
                    pSubscriptionList,
                    pTopicFilterString,
                    ( unsigned int ) topicFilterLength,
                    incomingPublishCallback ) );
    }
    else
    {
        /* Start at end of array, so that we will insert at the first available index.
         * Scans backwards to find duplicates. */
        for( index = ( int32_t ) SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS - 1; index >= 0; index-- )
        {
            if( pSubscriptionList[ index ].filterStringLength == 0 )
            {
                availableIndex = index;
            }
            else if( ( pSubscriptionList[ index ].filterStringLength == topicFilterLength ) &&
                     ( strncmp( pTopicFilterString, pSubscriptionList[ index ].pSubscriptionFilterString, ( size_t ) topicFilterLength ) == 0 ) )
            {
                /* If a subscription already exists, don't do anything. */
                if( ( pSubscriptionList[ index ].incomingPublishCallback == incomingPublishCallback ) &&
                    ( pSubscriptionList[ index ].pIncomingPublishCallbackContext == pIncomingPublishCallbackContext ) )
                {
                    LogWarn( ( "Subscription already exists.\n" ) );
                    availableIndex = SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS;
                    returnStatus = true;
                    break;
                }
            }
        }

        if( availableIndex < SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS )
        {
            pSubscriptionList[ availableIndex ].pSubscriptionFilterString = pTopicFilterString;
            pSubscriptionList[ availableIndex ].filterStringLength = topicFilterLength;
            pSubscriptionList[ availableIndex ].incomingPublishCallback = incomingPublishCallback;
            pSubscriptionList[ availableIndex ].pIncomingPublishCallbackContext = pIncomingPublishCallbackContext;
            returnStatus = true;
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

void removeSubscription( SubscriptionElement_t * pSubscriptionList,
                         const char * pTopicFilterString,
                         uint16_t topicFilterLength )
{
    int32_t index = 0;

    if( ( pSubscriptionList == NULL ) ||
        ( pTopicFilterString == NULL ) ||
        ( topicFilterLength == 0U ) )
    {
        LogError( ( "Invalid parameter. pSubscriptionList=%p, pTopicFilterString=%p,"
                    " topicFilterLength=%u.",
                    pSubscriptionList,
                    pTopicFilterString,
                    ( unsigned int ) topicFilterLength ) );
    }
    else
    {
        for( index = 0; index < SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS; index++ )
        {
            if( pSubscriptionList[ index ].filterStringLength == topicFilterLength )
            {
                if( strncmp( pSubscriptionList[ index ].pSubscriptionFilterString, pTopicFilterString, topicFilterLength ) == 0 )
                {
                    memset( &( pSubscriptionList[ index ] ), 0x00, sizeof( SubscriptionElement_t ) );
                }
            }
        }
    }
}

/*-----------------------------------------------------------*/

bool handleIncomingPublishes( SubscriptionElement_t * pSubscriptionList,
                              MQTTPublishInfo_t * pPublishInfo )
{
    int32_t index = 0;
    bool isMatched = false, publishHandled = false;

    if( ( pSubscriptionList == NULL ) ||
        ( pPublishInfo == NULL ) )
    {
        LogError( ( "Invalid parameter. pSubscriptionList=%p, pPublishInfo=%p,",
                    pSubscriptionList,
                    pPublishInfo ) );
    }
    else
    {
        for( index = 0; index < SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS; index++ )
        {
            if( pSubscriptionList[ index ].filterStringLength > 0 )
            {
                MQTT_MatchTopic( pPublishInfo->pTopicName,
                                 pPublishInfo->topicNameLength,
                                 pSubscriptionList[ index ].pSubscriptionFilterString,
                                 pSubscriptionList[ index ].filterStringLength,
                                 &isMatched );

                if( isMatched == true )
                {
                    pSubscriptionList[ index ].incomingPublishCallback( pSubscriptionList[ index ].pIncomingPublishCallbackContext,
                                                                            pPublishInfo );
                    publishHandled = true;
                }
            }
        }
    }

    return publishHandled;
}
