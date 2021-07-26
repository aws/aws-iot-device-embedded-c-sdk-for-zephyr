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
#include <assert.h>
#include <string.h>
#include <errno.h>

/* Zephyr socket includes. */
#include <net/socket.h>

#include "plaintext_zephyr.h"

/*-----------------------------------------------------------*/

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    PlaintextParams_t * pParams;
};

/*-----------------------------------------------------------*/

/**
 * @brief Log possible error from send/recv.
 *
 * @param[in] errorNumber Error number to be logged.
 */
static void logTransportError( int32_t errorNumber );

/*-----------------------------------------------------------*/

static void logTransportError( int32_t errorNumber )
{
    /* Remove unused parameter warning. */
    ( void ) errorNumber;

    LogError( ( "A transport error occurred: %d.", errorNumber ) );
}
/*-----------------------------------------------------------*/

SocketStatus_t Plaintext_Connect( NetworkContext_t * pNetworkContext,
                                  const ServerInfo_t * pServerInfo,
                                  uint32_t sendTimeoutMs,
                                  uint32_t recvTimeoutMs )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    PlaintextParams_t * pPlaintextParams = NULL;

    /* Validate parameters. */
    if( ( pNetworkContext == NULL ) || ( pNetworkContext->pParams == NULL ) )
    {
        LogError( ( "Parameter check failed: pNetworkContext is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        pPlaintextParams = pNetworkContext->pParams;
        returnStatus = Sockets_Connect( &pPlaintextParams->socketDescriptor,
                                        pServerInfo,
                                        sendTimeoutMs,
                                        recvTimeoutMs );
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

SocketStatus_t Plaintext_Disconnect( const NetworkContext_t * pNetworkContext )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    PlaintextParams_t * pPlaintextParams = NULL;

    /* Validate parameters. */
    if( ( pNetworkContext == NULL ) || ( pNetworkContext->pParams == NULL ) )
    {
        LogError( ( "Parameter check failed: pNetworkContext is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        pPlaintextParams = pNetworkContext->pParams;
        returnStatus = Sockets_Disconnect( pPlaintextParams->socketDescriptor );
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

int32_t Plaintext_Recv( NetworkContext_t * pNetworkContext,
                        void * pBuffer,
                        size_t bytesToRecv )
{
    PlaintextParams_t * pPlaintextParams = NULL;
    int32_t bytesReceived = -1, pollStatus = 1;
    struct zsock_pollfd pollFds;

    assert( pNetworkContext != NULL && pNetworkContext->pParams != NULL );
    assert( pBuffer != NULL );
    assert( bytesToRecv > 0 );

    pPlaintextParams = pNetworkContext->pParams;

    /* Initialize the file descriptor.
     * #ZSOCK_POLLPRI corresponds to high-priority data while #ZSOCK_POLLIN corresponds
     * to any other data that may be read. */
    pollFds.events = ZSOCK_POLLIN | ZSOCK_POLLPRI;
    pollFds.revents = 0;
    /* Set the file descriptor for poll. */
    pollFds.fd = pPlaintextParams->socketDescriptor;

    /* Speculative read for the start of a payload.
     * Note: This is done to avoid blocking when
     * no data is available to be read from the socket. */
    if( bytesToRecv == 1U )
    {
        /* Check if there is data to read (without blocking) from the socket.
         * Note: A timeout value of zero causes zsock_poll to not detect data on the socket
         * even across multiple re-tries. Thus, the smallest non-zero block time of 1ms is used. */
        pollStatus = zsock_poll( &pollFds, 1, 1 );
    }

    if( pollStatus > 0 )
    {
        /* The socket is available for receiving data. */
        bytesReceived = ( int32_t ) zsock_recv( pPlaintextParams->socketDescriptor,
                                                pBuffer,
                                                bytesToRecv,
                                                0 );
    }
    else if( pollStatus < 0 )
    {
        /* An error occurred while polling. */
        bytesReceived = -1;
    }
    else
    {
        /* No data available to receive. */
        bytesReceived = 0;
    }

    /* Note: A zero value return from recv() represents
     * closure of TCP connection by the peer. */
    if( ( pollStatus > 0 ) && ( bytesReceived == 0 ) )
    {
        /* Peer has closed the connection. Treat as an error. */
        bytesReceived = -1;
    }
    else if( bytesReceived < 0 )
    {
        logTransportError( errno );
    }

    return bytesReceived;
}
/*-----------------------------------------------------------*/

int32_t Plaintext_Send( NetworkContext_t * pNetworkContext,
                        const void * pBuffer,
                        size_t bytesToSend )
{
    PlaintextParams_t * pPlaintextParams = NULL;
    int32_t bytesSent = -1, pollStatus = -1;
    struct zsock_pollfd pollFds;

    assert( pNetworkContext != NULL && pNetworkContext->pParams != NULL );
    assert( pBuffer != NULL );
    assert( bytesToSend > 0 );

    pPlaintextParams = pNetworkContext->pParams;

    /* Initialize the file descriptor. */
    pollFds.events = ZSOCK_POLLOUT;
    pollFds.revents = 0;
    /* Set the file descriptor for poll. */
    pollFds.fd = pPlaintextParams->socketDescriptor;

    /* Check if data can be written to the socket.
     * Note: This is done to avoid blocking on send() when
     * the socket is not ready to accept more data for network
     * transmission (possibly due to a full TX buffer). */
    pollStatus = zsock_poll( &pollFds, 1, 0 );

    if( pollStatus > 0 )
    {
        /* The socket is available for sending data. */
        bytesSent = ( int32_t ) zsock_send( pPlaintextParams->socketDescriptor,
                                            pBuffer,
                                            bytesToSend,
                                            0 );
    }
    else if( pollStatus < 0 )
    {
        /* An error occurred while polling. */
        bytesSent = -1;
    }
    else
    {
        /* Socket is not available for sending data. */
        bytesSent = 0;
    }

    if( ( pollStatus > 0 ) && ( bytesSent == 0 ) )
    {
        /* Peer has closed the connection. Treat as an error. */
        bytesSent = -1;
    }
    else if( bytesSent < 0 )
    {
        logTransportError( errno );
    }

    return bytesSent;
}
/*-----------------------------------------------------------*/
