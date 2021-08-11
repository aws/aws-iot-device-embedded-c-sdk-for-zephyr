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

/* Zephyr Includes. */
#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <logging/log.h>

/* Espressif ESP-IDF Includes. */
#include <esp_wifi.h>

#include "esp_wifi_wrapper.h"

LOG_MODULE_REGISTER( espressifWifiStation, LOG_LEVEL_DBG );

/**
 * @brief Intial count for wifiSem. Starts at 0 we block on taking the semaphore
 * until it is given by a successful connection.
 */
#define WIFI_SEMAPHORE_INITIAL    ( 0 )

/**
 * @brief Maximum count for wifiSem.
 */
#define WIFI_SEMAPHORE_LIMIT      ( 1 )

/**
 * @brief Semaphore to block execution until board is finished connecting to wifi.
 */
static struct k_sem wifiSem;

static struct net_mgmt_event_callback dhcpCb;

/*-----------------------------------------------------------*/

/**
 * @brief Callback that is invoked on a successful connection to WiFi network.
 *
 * @param[in] pEventCb Original struct net_mgmt_event_callback owning this handler.
 * @param[in] mgmtEvent The network event being notified.
 * @param[in] pInterface A pointer on a struct net_if to which the the event belongs to, if it’s an event on an interface. NULL otherwise.
 */
static void wifiConnectionCallback( struct net_mgmt_event_callback * pEventCb,
                                    uint32_t mgmtEvent,
                                    struct net_if * pInterface );

/*-----------------------------------------------------------*/

static void wifiConnectionCallback( struct net_mgmt_event_callback * pEventCb,
                                    uint32_t mgmtEvent,
                                    struct net_if * pInterface )
{
    if( mgmtEvent == NET_EVENT_IPV4_DHCP_BOUND )
    {
        char ipAddr[ NET_IPV4_ADDR_LEN ];

        LOG_INF( "Your address: %s\n",
                 log_strdup( net_addr_ntop( AF_INET,
                                            &pInterface->config.dhcpv4.requested_ip,
                                            ipAddr, sizeof( ipAddr ) ) ) );
        LOG_INF( "Lease time: %u seconds\n",
                 pInterface->config.dhcpv4.lease_time );
        LOG_INF( "Subnet: %s\n",
                 log_strdup( net_addr_ntop( AF_INET,
                                            &pInterface->config.ip.ipv4->netmask,
                                            ipAddr, sizeof( ipAddr ) ) ) );
        LOG_INF( "Router: %s\n",
                 log_strdup( net_addr_ntop( AF_INET,
                                            &pInterface->config.ip.ipv4->gw,
                                            ipAddr, sizeof( ipAddr ) ) ) );

        /* Wifi successfully connected, so give semaphore. */
        k_sem_give( &wifiSem );
    }
}
/*-----------------------------------------------------------*/

bool Wifi_Connect( const char * pWiFiSsid,
                   size_t ssidLen,
                   const char * pWiFiPassword,
                   size_t passwordLen )
{
    struct net_if * pInterface = NULL;
    esp_err_t wifiStatus = esp_wifi_set_mode( WIFI_MODE_STA );

    if( wifiStatus != ESP_OK )
    {
        LOG_ERR( "Failed to set WiFi operating mode: ReturnCode=%d", wifiStatus );
    }
    else
    {
        /* Initialize a semaphore for blocking until connected */
        k_sem_init( &wifiSem, WIFI_SEMAPHORE_INITIAL, WIFI_SEMAPHORE_LIMIT );

        /* Initializes the struct net_mgmt_event_callback, and registers it.
         * The function wifiConnectionCallback will be invoked when the NET_EVENT_IPV4_DHCP_BOUND event is set */
        net_mgmt_init_event_callback( &dhcpCb, wifiConnectionCallback,
                                      NET_EVENT_IPV4_DHCP_BOUND );
        /* TODO: IPV6 support */
        net_mgmt_add_event_callback( &dhcpCb );

        /* Gets Zephyr's default network interface */
        pInterface = net_if_get_default();

        if( pInterface != NULL )
        {
            /* Starts DHCPv4 client on pInterface and begins negotiating for IPv4 address. */
            net_dhcpv4_start( pInterface );

            if( !IS_ENABLED( CONFIG_ESP32_WIFI_STA_AUTO ) )
            {
                wifi_config_t wifi_config = { 0 };

                memcpy( wifi_config.sta.ssid, pWiFiSsid, ssidLen );
                memcpy( wifi_config.sta.password, pWiFiPassword, passwordLen );

                /* Set the configuration of the ESP32 station and connect to network. */
                wifiStatus |= esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config );
                wifiStatus |= esp_wifi_connect();

                if( wifiStatus != ESP_OK )
                {
                    LOG_ERR( "Failed to connect to WiFi network: SSID=%.*s, ReturnCode=%d", ssidLen, pWiFiSsid, wifiStatus );
                }
            }

            /* Take a semaphore, blocking until semaphore is given from successful connection */
            k_sem_take( &wifiSem, K_FOREVER );
        }
        else
        {
            LOG_ERR( "wifi interface not available" );
            wifiStatus = ESP_FAIL;
        }
    }

    return wifiStatus == ESP_OK;
}
