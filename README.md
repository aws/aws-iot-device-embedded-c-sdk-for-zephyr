# zephyr-c-sdk
SDK for connecting to AWS IoT from a device using embedded C. This repository contains only the demos and parts for Zephyr.

## Overview
This repository is a port of AWS IoT Device SDK for Embedded C (C-SDK) for Zephyr RTOS. See https://github.com/aws/aws-iot-device-sdk-embedded-C for main documentation.

## Getting Started

### Cloning

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

The libraries in this SDK are not dependent on any operating system. However, the demos for the libraries in this SDK are built and tested on the Zephyr platform. The demos build with [west](https://docs.zephyrproject.org/latest/guides/west/index.html), Zephyr's meta-tool.


