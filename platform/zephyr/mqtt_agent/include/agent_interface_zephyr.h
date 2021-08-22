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

#ifndef AGENT_INTERFACE_ZEPHYR_H_
#define AGENT_INTERFACE_ZEPHYR_H_

/* Kernel Header */
#include <zephyr.h>

/* coreMQTT Agent interface includes */
#include "core_mqtt_agent_message_interface.h"
#include "core_mqtt_agent.h"

/**
 * @ingroup mqtt_agent_struct_types
 * @brief Context with which tasks may deliver messages to the agent.
 */
struct MQTTAgentMessageContext
{
    struct k_msgq queue;
};

/**
 * @brief Send a message to the specified context.
 * Must be thread safe.
 *
 * @param[in] pMsgCtx An #MQTTAgentMessageContext_t.
 * @param[in] pCommandToSend Pointer to address to send to queue.
 * @param[in] blockTimeMs Block time to wait for a send.
 *
 * @return `true` if send was successful, else `false`.
 */
bool Agent_MessageSend( MQTTAgentMessageContext_t * pMsgCtx,
                        MQTTAgentCommand_t * const * pCommandToSend,
                        uint32_t blockTimeMs );

/**
 * @brief Receive a message from the specified context.
 * Must be thread safe.
 *
 * @param[in] pMsgCtx An #MQTTAgentMessageContext_t.
 * @param[in] pReceivedCommand Pointer to write address of received command.
 * @param[in] blockTimeMs Block time to wait for a receive.
 *
 * @return `true` if receive was successful, else `false`.
 */
bool Agent_MessageReceive( MQTTAgentMessageContext_t * pMsgCtx,
                           MQTTAgentCommand_t ** pReceivedCommand,
                           uint32_t blockTimeMs );

/**
 * @brief Initialize the common task pool. Not thread safe, but only called on one thread.
 */
void Agent_InitializePool( void );

/**
 * @brief Obtain a MQTTAgentCommand_t structure.
 *
 * @note MQTTAgentCommand_t structures hold everything the MQTT agent needs to process a
 * command that originates from application.  Examples of commands are PUBLISH and
 * SUBSCRIBE. The MQTTAgentCommand_t structure must persist for the duration of the command's
 * operation.
 *
 * @param[in] blockTimeMs The length of time the calling task should remain in the
 * Blocked state (so not consuming any CPU time) to wait for a MQTTAgentCommand_t structure to
 * become available should one not be immediately at the time of the call.
 *
 * @return A pointer to a MQTTAgentCommand_t structure if one becomes available before
 * blockTimeMs time expired, otherwise NULL.
 */
MQTTAgentCommand_t * Agent_GetCommand( uint32_t blockTimeMs );

/**
 * @brief Free a MQTTAgentCommand_t structure.
 *
 * @note MQTTAgentCommand_t structures hold everything the MQTT agent needs to process a
 * command that originates from application.  Examples of commands are PUBLISH and
 * SUBSCRIBE.  The MQTTAgentCommand_t structure must persist for the duration of the command's
 * operation.
 *
 * @param[in] pCommandToRelease A pointer to the MQTTAgentCommand_t structure to free.
 * The structure must first have been obtained by calling Agent_GetCommand(), otherwise
 * Agent_ReleaseCommand() will have no effect.
 *
 * @return true if the MQTTAgentCommand_t structure was freed, otherwise false.
 */
bool Agent_FreeCommand( MQTTAgentCommand_t * pCommandToRelease );

#endif /* ifndef AGENT_INTERFACE_ZEPHYR_H_ */
