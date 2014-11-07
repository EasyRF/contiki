#Contiki sources for the Evaluation kit for ERF1000 module


The ERF1000 module and its Evaluation Kit feature the lastest
ARM Cortex-M0+ CPU with integrated 2.4 GHz transceiver.

The module and kit are especially designed with IoT projects in mind.

For more information or to buy a kit or module, see the EasyRF website:

[http://shop.top-electronics.eu/evaluation-kit-for-erf1000-module-p-17199.html]()

For more information about Contiki, see the Contiki website:

[http://contiki-os.org]()

##Getting started

Contiki has a special directory for cpu related files and for platform related files.
For the ERF1000 evalution kit the cpu files can be found in
`contiki/cpu/atsamr21` and the platform files in `contiki/platform/easyRF` and the demo's are in `contiki/examples/easyRF`.

To get up and running as quickly as possible follow the steps below.

### Install tools


To run Contiki on the Evaluation Kit for ERF1000 you will need to install a few things.

#### 1. GCC ARM Embedded

You need this to cross compile software for ARM CPU's. There are pre-build binaries for all major OS'es.

[https://launchpad.net/gcc-arm-embedded](GNU Tools for ARM Embedded Processors)

Make sure the bin directory (containing the executables) is in your __PATH__.

#### 2. Make environment

Most developers will already have make installed. But if not you need to install this.

###### OSX

Install the Command Line Tools you can find them here: 

[https://developer.apple.com/opensource/]() 

###### Linux

`sudo apt-get install build-essential`

###### Windows

Install the make package from GNUWin32 you can find it here: 

[http://gnuwin32.sourceforge.net/packages/make.htm]()

#### 3. Atmel Software Framework (ASF)

The CPU and platform drivers for the ERF1000 module are all written on top of the drivers in ASF. Download the latest version from:

[http://www.atmel.com/tools/AVRSOFTWAREFRAMEWORK.aspx]()

#### 4. OpenOCD (optional)

OpenOCD can be used for pogramming binaries into your target.
Its open source and available for all major OS'es. As of version 0.8.0 the target at91samr21g18 is supported. The latest source code can be downloaded from:

[http://sourceforge.net/projects/openocd/files/latest/download?source=files]()

Build instuctions are included in the download.

The `contiki/cpu/atsamr21/Makefile.atsamr21` contains a special build target for uploading the application to the target using openocd.

To use this target openocd must be in your __PATH__ or you can specify the path to openocd in `OPENOCD_PROG` in `contiki/cpu/atsamr21/Makefile.atsamr21`.

### Get the code

The easiest way to get sources is to install Git on your machine and clone the repository.

`git clone https://github.com/EasyRF/contiki.git`

Alternatively you can download the sources from GitHub as a ZIP-file.

Because the CPU and platfrom drivers depend on ASF (see step 3), you will need to unzip the ASF archive in contiki/thirdparty/atmel. For example: `contiki/thirdparty/atmel/xdk-asf-3.20.1` 

And make sure the `ASF_ROOT` variable in `contiki/cpu/atsamr21/Makefile.atsamr21` contains the correct path for the ASF library. The current code is developed/tested with version 3.20.1.

### Build and Run the demo

Now it's time to build the first example program.

Change to the directory containing the easyRF demo.

`cd contiki/examples/easyRF`

For a demo of all sensors and HTTP posting over a mesh network run:

`make TARGET=easyRF sensors-test`

To upload the demo run:

`make TARGET=easyRF sensors-test.ocd-upload`

