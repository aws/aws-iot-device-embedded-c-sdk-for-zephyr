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

#ifndef POSIX_AGENT_INTERFACE_H_
#define POSIX_AGENT_INTERFACE_H_

#include <unistd.h>
#include <pthread.h>

#include "core_mqtt_agent_message_interface.h"
#include "core_mqtt_agent.h"

#include "demo_queue.h"

struct MQTTAgentMessageContext
{
    DeQueue_t queue;
};

bool Agent_MessageSend( MQTTAgentMessageContext_t * pMsgCtx,
                       MQTTAgentCommand_t * const * pCommandToSend,
                       uint32_t blockTimeMs );

bool Agent_MessageReceive( MQTTAgentMessageContext_t * pMsgCtx,
                           MQTTAgentCommand_t ** pReceivedCommand,
                           uint32_t blockTimeMs );

MQTTAgentCommand_t * Agent_GetCommand( uint32_t blockTimeMs );
bool Agent_FreeCommand( MQTTAgentCommand_t * pCommandToRelease );

#endif /* ifndef DEMO_QUEUE_H_ */

