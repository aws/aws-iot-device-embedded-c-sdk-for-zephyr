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

/* Agent interface header */
#include "agent_interface_zephyr.h"

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

MQTTAgentCommand_t * Agent_GetCommand( uint32_t blockTimeMs )
{
    MQTTAgentCommand_t * pCommand = ( MQTTAgentCommand_t * ) malloc( sizeof( MQTTAgentCommand_t ) );

    ( void ) blockTimeMs;
    memset( pCommand, 0x00, sizeof( MQTTAgentCommand_t ) );
    return pCommand;
}
/*-----------------------------------------------------------*/

bool Agent_FreeCommand( MQTTAgentCommand_t * pCommandToRelease )
{
    free( pCommandToRelease );
    return true;
}
