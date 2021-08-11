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
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Establish WiFi connection with the passed information of network SSID and password.
 *
 * If the wifi interface is available, this function will block until connection is succesful.
 *
 * @param[in] pWifiSsid Wifi network name.
 * @param[in] ssidLen Length of wifi network name.
 * @param[in] pWifiPassword Password to wifi network if using WPA.
 * @param[in] passwordLen Length of password to wifi network.
 *
 * @return true on successful connection, false otherwise.
 */
bool Wifi_Connect( const char * pWiFiSsid,
                   size_t ssidLen,
                   const char * pWiFiPassword,
                   size_t passwordLen );
