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

/* Standard includes. */
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/* Agent interface header */
#include "agent_interface_zephyr.h"

/*-----------------------------------------------------------*/

/**
 * @brief The number of structures to allocate in the command pool.
 */
#ifndef NUM_COMMANDS_IN_POOL
    #define NUM_COMMANDS_IN_POOL    ( 10U )
#endif

/*-----------------------------------------------------------*/

/**
 * @brief The pool of command structures used to hold information on commands (such
 * as PUBLISH or SUBSCRIBE) between the command being created by an API call and
 * completion of the command by the execution of the command's callback.
 */
static MQTTAgentCommand_t commandStructurePool[ NUM_COMMANDS_IN_POOL ];

/**
 * @brief Statically create a message queue to handle the command structures.
 * The message queue is used for managing the memory blocks of command objects in the
 * command pool in a thread-safe manner.
 */
K_MSGQ_DEFINE( commandStructureQueue, sizeof( MQTTAgentCommand_t * ), NUM_COMMANDS_IN_POOL, sizeof( MQTTAgentCommand_t * ) );

/**
 * @brief Initialization status of the queue.
 */
static volatile uint8_t queueInit = false;

/*-----------------------------------------------------------*/

bool Agent_MessageSend( MQTTAgentMessageContext_t * pMsgCtx,
                        MQTTAgentCommand_t * const * pCommandToSend,
                        uint32_t blockTimeMs )
{
    bool ret = false;

    if( ( pMsgCtx != NULL ) && ( pCommandToSend != NULL ) )
    {
        ret = ( k_msgq_put( &( pMsgCtx->queue ), pCommandToSend, K_MSEC( blockTimeMs ) ) == 0 );
    }

    return ret;
}
/*-----------------------------------------------------------*/

bool Agent_MessageReceive( MQTTAgentMessageContext_t * pMsgCtx,
                           MQTTAgentCommand_t ** pReceivedCommand,
                           uint32_t blockTimeMs )
{
    bool ret = false;

    if( ( pMsgCtx != NULL ) && ( pReceivedCommand != NULL ) )
    {
        ret = ( k_msgq_get( &( pMsgCtx->queue ), pReceivedCommand, K_MSEC( blockTimeMs ) ) == 0 );
    }

    return ret;
}
/*-----------------------------------------------------------*/

void Agent_InitializePool( void )
{
    size_t i;
    MQTTAgentCommand_t * pCommand;
    bool commandAdded = false;

    if( !queueInit )
    {
        /* Populate the queue. */
        for( i = 0; i < NUM_COMMANDS_IN_POOL; i++ )
        {
            /* Store the address as a variable. */
            pCommand = &commandStructurePool[ i ];
            /* Send the pointer to the queue. */
            commandAdded = ( k_msgq_put( &commandStructureQueue, &pCommand, K_NO_WAIT ) == 0 );
            assert( commandAdded );
        }

        queueInit = true;
    }
}
/*-----------------------------------------------------------*/

MQTTAgentCommand_t * Agent_GetCommand( uint32_t blockTimeMs )
{
    MQTTAgentCommand_t * structToUse = NULL;
    bool commandRetrieved = false;

    /* Check queue has been created. */
    assert( queueInit );

    commandRetrieved = ( k_msgq_get( &commandStructureQueue, &structToUse, K_MSEC( blockTimeMs ) ) == 0 );

    if( !commandRetrieved )
    {
        LogError( ( "No command structure available. Maximum number of commands statically allocated in the pool is: %d",
                    NUM_COMMANDS_IN_POOL ) );
    }

    return structToUse;
}
/*-----------------------------------------------------------*/

bool Agent_FreeCommand( MQTTAgentCommand_t * pCommandToRelease )
{
    bool structReturned = false;

    /* Check queue has been created. */
    assert( queueInit );

    /* See if the structure being returned is actually from the pool. */
    if( ( pCommandToRelease >= commandStructurePool ) &&
        ( pCommandToRelease < ( commandStructurePool + NUM_COMMANDS_IN_POOL ) ) )
    {
        structReturned = ( k_msgq_put( &commandStructureQueue, &pCommandToRelease, K_NO_WAIT ) == 0 );

        assert( structReturned );
    }

    return structReturned;
}
