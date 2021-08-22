# AWS IoT Device Embedded C SDK for Zephyr

## Table of Contents

* [Overview](#overview)
    * [License](#license)
    * [Features](#features)
        * [coreMQTT](#coremqtt)
        * [coreHTTP](#corehttp)
        * [coreJSON](#corejson)
        * [AWS IoT Device Shadow](#aws-iot-device-shadow)
        * [backoffAlgorithm](#backoffalgorithm)
    * [Sending metrics to AWS IoT](#sending-metrics-to-aws-iot)
* [Porting Guide for 202009.00 and newer releases](#porting-guide-for-20200900-and-newer-releases)
    * [Porting coreMQTT](#porting-coremqtt)
    * [Porting coreHTTP](#porting-corehttp)
    * [Porting AWS IoT Device Shadow](#porting-aws-iot-device-shadow)
* [Getting Started](#getting-started)
    * [1. Setting Up Zepyhr](#1-setup-zephyr)
    * [2. Setting ESP32](#2-setup-esp32)
    * [3. Clone This Repository](#3-clone-this-repository)
    * [4. Configuring Demos](#4-configuring-demos)
         * [Wi-Fi Connection Setup](#wi-fi-connection-setup)
         * [AWS IoT Account Setup](#aws-iot-account-setup)
         * [Configuring mutual authentication demos of MQTT and HTTP](#configuring-mutual-authentication-demos-of-mqtt-and-http)
         * [Configuring AWS IoT Device Shadow demo](#configuring-aws-iot-device-shadow-demo)
    * [5. Building and Flashing Demos](#5-building-and-flashing-demos)
* [Adding C-SDK to Zephyr Application](#adding-c-sdk-to-zephyr-application)



## Overview

This repository is a port of AWS IoT Device SDK for Embedded C (C-SDK) for Zephyr RTOS. The AWS IoT Device SDK for Embedded C (C-SDK) is a collection of C source files under the [MIT open source license](LICENSE) that can be used in embedded applications to securely connect IoT devices to [AWS IoT Core](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html). It contains MQTT client, HTTP client, JSON Parser, and AWS IoT Device Shadow libraries. See https://github.com/aws/aws-iot-device-sdk-embedded-C for main documentation.

The libraries in the SDK are not dependent on any operating system. However, the demos for the libraries in this SDK are built and tested on the Zephyr platform for the ESP32 board. The demos build with [west](https://docs.zephyrproject.org/latest/guides/west/index.html), Zephyr's meta-tool.

**C-SDK includes libraries that are part of the [FreeRTOS 202012.01 LTS](https://github.com/FreeRTOS/FreeRTOS-LTS/tree/202012.01-LTS) release. Learn more about the FreeRTOS 202012.01 LTS libraries by [clicking here](https://freertos.org/lts-libraries.html).**

### License

The C-SDK libraries are licensed under the [MIT open source license](LICENSE).

### Features

C-SDK simplifies access to various AWS IoT services. C-SDK has been tested to work with [AWS IoT Core](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html) and an open source MQTT broker to ensure interoperability. The AWS IoT Device Shadow library is flexible to work with any MQTT client and JSON parser. The MQTT client and JSON parser libraries are offered as choices without being tightly coupled with the rest of the SDK. This C-SDK for Zephyr contains the following libraries:

#### coreMQTT

The [coreMQTT](https://github.com/FreeRTOS/coreMQTT) library provides the ability to establish an MQTT connection with a broker over a customer-implemented transport layer, which can either be a secure channel like a TLS session (mutually authenticated or server-only authentication) or a non-secure channel like a plaintext TCP connection. This MQTT connection can be used for publishing and subscribing to MQTT topics. The library provides a mechanism to register customer-defined callbacks for receiving incoming PUBLISH, acknowledgement and keep-alive response events from the broker. The library has been refactored for memory optimization and is compliant with the [MQTT 3.1.1](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) standard. It has no dependencies on any additional libraries other than the standard C library, a customer-implemented network transport interface, and optionally a customer-implemented platform time function. The refactored design embraces different use-cases, ranging from resource-constrained platforms using only QoS 0 MQTT PUBLISH messages to resource-rich platforms using QoS 2 MQTT PUBLISH over TLS connections.

See memory requirements for the latest release [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/coreMQTT/docs/doxygen/output/html/index.html#mqtt_memory_requirements).

#### coreHTTP

The [coreHTTP](https://github.com/FreeRTOS/coreHTTP) library provides the ability to establish an HTTP connection with a server over a customer-implemented transport layer, which can either be a secure channel like a TLS session (mutually authenticated or server-only authentication) or a non-secure channel like a plaintext TCP connection. The HTTP connection can be used to make "GET" (include range requests), "PUT", "POST" and "HEAD" requests. The library provides a mechanism to register a customer-defined callback for receiving parsed header fields in an HTTP response. The library has been refactored for memory optimization, and is a client implementation of a subset of the [HTTP/1.1](https://tools.ietf.org/html/rfc2616) standard.

See memory requirements for the latest release [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/coreHTTP/docs/doxygen/output/html/index.html#http_memory_requirements).

#### coreJSON

The [coreJSON](https://github.com/FreeRTOS/coreJSON) library is a JSON parser that strictly enforces the [ECMA-404 JSON standard](https://www.json.org/json-en.html). It provides a function to validate a JSON document, and a function to search for a key and return its value. A search can descend into nested structures using a compound query key. A JSON document validation also checks for illegal UTF8 encodings and illegal Unicode escape sequences.

See memory requirements for the latest release [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/coreJSON/docs/doxygen/output/html/index.html#json_memory_requirements).

#### AWS IoT Device Shadow

The AWS IoT Device Shadow library enables you to retrieve and update the current state of one or more shadows of every registered device. A device’s shadow is a persistent, virtual representation of your device that you can interact with from AWS IoT Core even if the device is offline. The device state is captured in its "shadow" is represented as a [JSON](https://www.json.org/) document. The device can send commands over MQTT to get, update and delete its latest state as well as receive notifications over MQTT about changes in its state. The device’s shadow(s) are uniquely identified by the name of the corresponding "thing", a representation of a specific device or logical entity on the AWS Cloud. See [Managing Devices with AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/iot-thing-management.html) for more information on IoT "thing". This library supports named shadows, a feature of the AWS IoT Device Shadow service that allows you to create multiple shadows for a single IoT device. More details about AWS IoT Device Shadow can be found in [AWS IoT documentation](https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html).

The AWS IoT Device Shadow library has no dependencies on additional libraries other than the standard C library. It also doesn’t have any platform dependencies, such as threading or synchronization. It can be used with any MQTT library and any JSON library (see [demos](demos/shadow) with coreMQTT and coreJSON).

See memory requirements for the latest release [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/aws/device-shadow-for-aws-iot-embedded-sdk/docs/doxygen/output/html/index.html#shadow_memory_requirements).

#### backoffAlgorithm

The [backoffAlgorithm](https://github.com/FreeRTOS/backoffAlgorithm) library is a utility library to calculate backoff period using an exponential backoff with jitter algorithm for retrying network operations (like failed network connection with server). This library uses the "Full Jitter" strategy for the exponential backoff with jitter algorithm. More information about the algorithm can be seen in the [Exponential Backoff and Jitter AWS blog](https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/).

Exponential backoff with jitter is typically used when retrying a failed connection or network request to the server. An exponential backoff with jitter helps to mitigate the failed network operations with servers, that are caused due to network congestion or high load on the server, by spreading out retry requests across multiple devices attempting network operations. Besides, in an environment with poor connectivity, a client can get disconnected at any time. A backoff strategy helps the client to conserve battery by not repeatedly attempting reconnections when they are unlikely to succeed.

The backoffAlgorithm library has no dependencies on libraries other than the standard C library.

See memory requirements for the latest release [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/backoffAlgorithm/docs/doxygen/output/html/index.html#backoff_algorithm_memory_requirements).

### Sending metrics to AWS IoT

When establishing a connection with AWS IoT, users can optionally report the Operating System, Hardware Platform and MQTT client version information of their device to AWS. This information can help AWS IoT provide faster issue resolution and technical support. If users want to report this information, they can send a specially formatted string (see below) in the username field of the MQTT CONNECT packet.

Format

The format of the username string with metrics is:

```
<Actual_Username>?SDK=<OS_Name>&Version=<OS_Version>&Platform=<Hardware_Platform>&MQTTLib=<MQTT_Library_name>@<MQTT_Library_version>
```

Where

* <Actual_Username> is the actual username used for authentication, if username and password are used for authentication. When username and password based authentication is not used, this
is an empty value.
* <OS_Name> is the Operating System the application is running on (e.g. Zephyr)
* <OS_Version> is the version number of the Operating System (e.g. 2.6.0)
* <Hardware_Platform> is the Hardware Platform the application is running on (e.g. ESP32)
* <MQTT_Library_name> is the MQTT Client library being used (e.g. coreMQTT)
* <MQTT_Library_version> is the version of the MQTT Client library being used (e.g. 1.1.0)

Example

*  Actual_Username = “iotuser”, OS_Name = Zephyr, OS_Version = 2.6.0, Hardware_Platform_Name = ESP32, MQTT_Library_Name = coremqtt, MQTT_Library_version = 1.1.0. If username is not used, then “iotuser” can be removed.

```
/* Username string:
 * iotuser?SDK=Zephyr&Version=2.6.0&Platform=ESP32&MQTTLib=coremqtt@1.1.0
 */

#define OS_NAME                   "Zephyr"
#define OS_VERSION                "2.6.0"
#define HARDWARE_PLATFORM_NAME    "ESP32"
#define MQTT_LIB                  "coremqtt@1.1.0"

#define USERNAME_STRING           "iotuser?SDK=" OS_NAME "&Version=" OS_VERSION "&Platform=" HARDWARE_PLATFORM_NAME "&MQTTLib=" MQTT_LIB
#define USERNAME_STRING_LENGTH    ( ( uint16_t ) ( sizeof( USERNAME_STRING ) - 1 ) )

MQTTConnectInfo_t connectInfo;
connectInfo.pUserName = USERNAME_STRING;
connectInfo.userNameLength = USERNAME_STRING_LENGTH;
mqttStatus = MQTT_Connect( pMqttContext, &connectInfo, NULL, CONNACK_RECV_TIMEOUT_MS, pSessionPresent );
```

## Porting Guide for 202009.00 and newer releases

All libraries depend on the ISO C90 standard library and additionally on the `stdint.h` library for fixed-width integers, including `uint8_t`, `int8_t`, `uint16_t`, `uint32_t` and `int32_t`, and constant macros like `UINT16_MAX`. If your platform does not support the `stdint.h` library, definitions of the mentioned fixed-width integer types will be required for porting any C-SDK library to your platform.

### Porting coreMQTT

Guide for porting coreMQTT library to your platform is available [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/coreMQTT/docs/doxygen/output/html/mqtt_porting.html).

### Porting coreHTTP

Guide for porting coreHTTP library is available [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/standard/coreHTTP/docs/doxygen/output/html/http_porting.html).

### Porting AWS IoT Device Shadow

Guide for porting AWS IoT Device Shadow library is available [here](https://docs.aws.amazon.com/embedded-csdk/202103.00/lib-ref/libraries/aws/device-shadow-for-aws-iot-embedded-sdk/docs/doxygen/output/html/shadow_porting.html).

## Getting Started

### 1. Setup Zephyr

To setup Zephyr, please follow Zephyr's documentation: https://docs.zephyrproject.org/latest/getting_started/index.html

These steps should setup a Zephyr workspace with a root directory named "zephyrproject"

### 2. Setup ESP32

To setup development for the ESP32 board, please follow Zephyr's documentation for the board: https://docs.zephyrproject.org/latest/boards/xtensa/esp32/doc/index.html

### 3. Clone This Repository

This repository may be cloned anywhere, not necessarily inside a Zephyr workspace.

This repository uses [Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to bring in the C-SDK libraries (eg, MQTT ) and third-party dependencies (eg, mbedtls for Zephyr platform transport layer).
Note: If you download the ZIP file provided by GitHub UI, you will not get the contents of the submodules (The ZIP file is also not a valid git repository).
To clone the latest commit to main branch using HTTPS:

```sh
git clone --recurse-submodules https://github.com/gilbefan/zephyr-c-sdk.git
```

Using SSH:

```sh
git clone --recurse-submodules git@github.com:gilbefan/zephyr-c-sdk.git
```

If you have downloaded the repo without using the `--recurse-submodules` argument, you need to run:

```sh
git submodule update --init --recursive
```

### 4. Configuring Demos

#### Wi-Fi Connection Setup

All demos in this repository will to be configured to connect to Wi-Fi. Edit `demo_config.h` in the desired demo's folder to `#define` the following:

* Set `WIFI_NETWORK_SSID` to the name of the Wi-Fi network to connect to.
* Set `WIFI_NETWORK_PASSWORD` to the password of the Wi-Fi network to connect to.

#### AWS IoT Account Setup

You need to setup an AWS account and access the AWS IoT console for running the AWS IoT Device Shadow library demos.
Also, the AWS account can be used for running the MQTT mutual auth demo against AWS IoT broker.
Note that running the AWS IoT Device Shadow library demo requires the setup of a Thing resource for the device running the demo.
Follow the links to:
- [Setup an AWS account](https://portal.aws.amazon.com/billing/signup#/start).
- [Sign-in to the AWS IoT Console](https://console.aws.amazon.com/iot/home) after setting up the AWS account.
- [Create a Thing resource](https://docs.aws.amazon.com/iot/latest/developerguide/iot-moisture-create-thing.html).

The MQTT Mutual Authentication and AWS IoT Shadow demos include example AWS IoT policy documents to run each respective demo with AWS IoT. You may use the [MQTT Mutual auth](./demos/mqtt/mqtt_demo_mutual_auth/aws_iot_policy_example_mqtt.json) and [Shadow](./demos/shadow/shadow_demo_main/aws_iot_policy_example_shadow.json) example policies by replacing `[AWS_REGION]` and `[AWS_ACCOUNT_ID]` with the strings of your region and account identifier. While the IoT Thing name and MQTT client identifier do not need to match for the demos to run, the example policies have the Thing name and client identifier identical as per [AWS IoT best practices](https://docs.aws.amazon.com/iot/latest/developerguide/security-best-practices.html).

It can be very helpful to also have the [AWS Command Line Interface tooling](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-install.html) installed.

#### Configuring mutual authentication demos of MQTT and HTTP

Edit `demo_config.h` in `demos/mqtt/mqtt_mutual_auth/` to `#define` the following:

* Set `AWS_IOT_ENDPOINT` to your custom endpoint. This is found on the *Settings* page of the AWS IoT Console and has a format of `ABCDEFG1234567.iot.<aws-region>.amazonaws.com` where `<aws-region>` can be an AWS region like `us-east-2`.  
   * Optionally, it can also be found with the AWS CLI command `aws iot describe-endpoint --endpoint-type iot:Data-ATS`.
* Set `ROOT_CA_CERT_PEM` to the root CA certificate downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup), if it is different from the default [AmazonRootCA1.pem](https://www.amazontrust.com/repository/AmazonRootCA1.pem). It is also possible to configure `ROOT_CA_CERT_PEM` to any PEM-encoded Root CA Certificate. A different Amazon Root CA certificate can be downloaded from [here](https://www.amazontrust.com/repository/). 
* Set `CLIENT_CERT_PEM` to the client certificate downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).
* Set `CLIENT_PRIVATE_KEY_PEM` to the private key downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).

#### Configuring AWS IoT Device Shadow demo

Edit `demo_config.h` in `demos/mqtt/mqtt_mutual_auth/` and `demos/http/http_mutual_auth/` to `#define` the following:

* Set `AWS_IOT_ENDPOINT` to your custom endpoint. This is found on the *Settings* page of the AWS IoT Console and has a format of `ABCDEFG1234567.iot.us-east-2.amazonaws.com`.
* Set `ROOT_CA_CERT_PEM` to the root CA certificate downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup), if it is different from the default [AmazonRootCA1.pem](https://www.amazontrust.com/repository/AmazonRootCA1.pem). A different Amazon Root CA certificate can be downloaded from [here](https://www.amazontrust.com/repository/). 
* Set `CLIENT_CERT_PEM` to the client certificate downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).
* Set `CLIENT_PRIVATE_KEY_PEM` to the private key downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).
* Set `THING_NAME` to the name of the Thing created in [AWS IoT Account Setup](#aws-iot-account-setup).

### 5. Building and Flashing Demos

1. Make sure the build environemnt variables are properly set up for the ESP32 by running these commands:

  On Linux and MacOS:

```
export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
export ESPRESSIF_TOOLCHAIN_PATH="${HOME}/.espressif/tools/xtensa-esp32-elf/esp-2020r3-8.4.0/xtensa-esp32-elf"
export PATH=$PATH:$ESPRESSIF_TOOLCHAIN_PATH/bin
```

  On Windows:

```
on CMD:
set ESPRESSIF_TOOLCHAIN_PATH=%USERPROFILE%\.espressif\tools\xtensa-esp32-elf\esp-2020r3-8.4.0\xtensa-esp32-elf
set ZEPHYR_TOOLCHAIN_VARIANT=espressif
set PATH=%PATH%;%ESPRESSIF_TOOLCHAIN_PATH%\bin

on PowerShell:
env:ESPRESSIF_TOOLCHAIN_PATH="$env:USERPROFILE\.espressif\tools\xtensa-esp32-elf\esp-2020r3-8.4.0\xtensa-esp32-elf"
env:ZEPHYR_TOOLCHAIN_VARIANT="espressif"
env:Path += "$env:ESPRESSIF_TOOLCHAIN_PATH\bin"
```

2. Make sure this and following steps are done inside the Zephyr workspace. Run `west update` followed by `west espressif update` to retrieve required Zephyr submodules for ESP32.
  
3. Run `west build -b esp32 file_path_to_demo` to build a demo, replacing `file_path_to_demo` with the path to the desired demo. (Note: if another demo had been previously built in the west workspace, the additional option `--pristine` may need to be added to the end of the command.

4. Run `west flash` to flash the demo. The option `--esp-device *ESP_DEVICE*`, where `*ESP_DEVICE*` is the serial port to flash, may also be useful to flash for ESP boards not connected to the default port. For documentation on additional options when flashing, please refer to https://docs.zephyrproject.org/latest/boards/xtensa/esp32/doc/index.html#flashing.

## Adding C-SDK to a Zephyr Application

To use C-SDK libraries in an existing Zephyr application, edit the CMakeLists.txt file of the application to target the library's sources and directories as defined in the `*.cmake` file in the library's root directory. For example, to add the coreMQTT library, add the following lines to CMakeLists.txt:

```
include( include( ${CSDK_BASE}/libraries/standard/coreMQTT/mqttFilePaths.cmake )

...

target_sources(
  ...
  ${MQTT_SOURCES}
  ${MQTT_SERIALIZER_SOURCES}
  ...
)

...

target_include_directories(
  ...
  ${MQTT_INCLUDE_PUBLIC_DIRS}
  ...
)

...
```
Where `${CSDK_BASE}` is the file path to the root of this repository.
