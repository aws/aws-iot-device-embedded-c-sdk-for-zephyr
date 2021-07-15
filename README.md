# zephyr-c-sdk

## Overview

This repository is a port of AWS IoT Device SDK for Embedded C (C-SDK) for Zephyr RTOS. See https://github.com/aws/aws-iot-device-sdk-embedded-C for main documentation.

The libraries in the SDK are not dependent on any operating system. However, the demos for the libraries in this SDK are built and tested on the Zephyr platform for the ESP32 board. The demos build with [west](https://docs.zephyrproject.org/latest/guides/west/index.html), Zephyr's meta-tool.

## Getting Started

### Zephyr

To setup Zephyr, please follow Zephyr's documentation: https://docs.zephyrproject.org/latest/getting_started/index.html

These steps should setup a Zephyr workspace with a root directory named "zephyrproject"

## ESP32

To setup development for the ESP32 board, please follow Zephyr's documentation for the board: https://docs.zephyrproject.org/latest/boards/xtensa/esp32/doc/index.html

### Cloning

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

### Configuring Demos

#### AWS IoT Account Setup

You need to setup an AWS account and access the AWS IoT console for running the MQTT mutual auth demo against AWS IoT broker.
Follow the links to:
- [Setup an AWS account](https://portal.aws.amazon.com/billing/signup#/start).
- [Sign-in to the AWS IoT Console](https://console.aws.amazon.com/iot/home) after setting up the AWS account.

It can be very helpful to also have the [AWS Command Line Interface tooling](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-install.html) installed.

#### Configuring MQTT mutual authentication demo

Edit `demo_config.h` in `demos/mqtt/mqtt_demo_mutual_auth/` to `#define` the following:

* Set `AWS_IOT_ENDPOINT` to your custom endpoint. This is found on the *Settings* page of the AWS IoT Console and has a format of `ABCDEFG1234567.iot.<aws-region>.amazonaws.com` where `<aws-region>` can be an AWS region like `us-east-2`.  
   * Optionally, it can also be found with the AWS CLI command `aws iot describe-endpoint --endpoint-type iot:Data-ATS`.
* Set `CLIENT_CERT_PATH` to the path of the client certificate downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).
* Set `CLIENT_PRIVATE_KEY_PATH` to the path of the private key downloaded when setting up the device certificate in [AWS IoT Account Setup](#aws-iot-account-setup).

It is possible to configure `ROOT_CA_CERT_PATH` to any PEM-encoded Root CA Certificate. However, this is optional because CMake will download and set it to [AmazonRootCA1.pem](https://www.amazontrust.com/repository/AmazonRootCA1.pem) when unspecified.

### Building and Flashing Demos

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

2. Make sure this and following steps are done inside the Zephyr workspace. Run `<west update>` followed by `<west espressif update>` to retrieve required Zephyr submodules for ESP32.
  
3. Run `<west build -b esp32 file_path_to_demo>` to build a demo, replacing `<file_path_to_demo>` with the path to the desired demo.

4. Run `<west flash>` to flash the demo. For documentation on additional options when flashing, please refer to https://docs.zephyrproject.org/latest/boards/xtensa/esp32/doc/index.html#flashing
