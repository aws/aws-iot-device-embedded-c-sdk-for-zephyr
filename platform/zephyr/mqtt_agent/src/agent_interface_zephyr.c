/*
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* POSIX includes. */
#include <unistd.h>
#include <pthread.h>

#include "posix_agent_interface.h"
#include "demo_queue.h"
#include "clock.h"

bool Agent_MessageSend( MQTTAgentMessageContext_t * pMsgCtx,
                        MQTTAgentCommand_t * const * pCommandToSend,
                        uint32_t blockTimeMs )
{
    bool ret = false;
    ( void ) blockTimeMs;
    if( ( pMsgCtx != NULL ) && ( pCommandToSend != NULL ) )
    {
        DeQueue_t * pQueue = &( pMsgCtx->queue );
        DeQueueElement_t * pNewElement = DeQueueElement_Create( *pCommandToSend, sizeof( MQTTAgentCommand_t * ), free );
        DeQueue_PushBack( pQueue, pNewElement );
        ret = true;
    }
    return ret;
}

bool Agent_MessageReceive( MQTTAgentMessageContext_t * pMsgCtx,
                           MQTTAgentCommand_t ** pReceivedCommand,
                           uint32_t blockTimeMs )
{
    bool ret = false;
    int counter = 0;
    if( ( pMsgCtx != NULL ) && ( pReceivedCommand != NULL ) )
    {
        DeQueue_t * pQueue = &( pMsgCtx->queue );
        DeQueueElement_t * pReceivedElement = NULL;
        MQTTAgentCommand_t * pCommand = NULL;
        for( counter = 0; counter < 2; counter++ )
        {
            pReceivedElement = DeQueue_PopFront( pQueue );
            if( pReceivedElement )
            {
                break;
            }
            if( counter == 0 )
            {
                Clock_SleepMs( blockTimeMs );
            }
        }
        if( pReceivedElement != NULL )
        {
            pCommand = ( MQTTAgentCommand_t * ) pReceivedElement->pData;
            *pReceivedCommand = pCommand;
            ret = true;
            free( pReceivedElement );
        }
    }
    return ret;
}

MQTTAgentCommand_t * Agent_GetCommand( uint32_t blockTimeMs )
{
    MQTTAgentCommand_t * pCommand = ( MQTTAgentCommand_t * ) malloc( sizeof( MQTTAgentCommand_t ) );
    ( void ) blockTimeMs;
    memset( pCommand, 0x00, sizeof( MQTTAgentCommand_t ) );
    return pCommand;
}

bool Agent_FreeCommand( MQTTAgentCommand_t * pCommandToRelease )
{
    free( pCommandToRelease );
    return true;
}

