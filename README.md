#Contiki sources for the Evaluation kit for ERF1000 module


The ERF1000 module and its Evaluation Kit feature the lastest
ARM Cortex-M0+ CPU with integrated 2.4 GHz transceiver.

For more information or to buy a kit or module, see the EasyRF website:

[http://shop.top-electronics.eu/evaluation-kit-for-erf1000-module-p-17199.html]()

For more information about Contiki, see the Contiki website:

[http://contiki-os.org]()

##Getting started

To get up and running as quickly as possible follow the steps below.

### Install tools


To run Contiki on the Evaluation Kit for ERF1000 you will need to install a few things.

#### 1. GCC ARM Embedded

You need this to cross compile software for ARM CPU's. There are pre-build binaries for all major OS'es.

[https://launchpad.net/gcc-arm-embedded](GNU Tools for ARM Embedded Processors)

Make sure the bin directory (containing all tools) is in your PATH

#### 2. Make environment

Most developers will already have make installed. But if not you need to install this.

###### OSX

Install the Command Line Tools you can find them here: [https://developer.apple.com/opensource/]() 

###### Linux

From a terminal run:
`sudo apt-get install build-essential`

###### Windows

Install the make package from GNUWin32 you can find it here: [http://gnuwin32.sourceforge.net/packages/make.htm]()

#### 3. Atmel Software Framework (ASF)

The CPU and platform drivers for the ERF1000 module are all written on top of the drivers in ASF. Download the latest version from:

[http://www.atmel.com/tools/AVRSOFTWAREFRAMEWORK.aspx]()

#### 4. OpenOCD (optional)

OpenOCD can be used for pogramming binaries into your target.
Its open source and available for all major OS'es. As of version 0.8.0 the target at91samr21g18 is supported. The latest source code can be downloaded from: [http://sourceforge.net/projects/openocd/files/latest/download?source=files]()

Build instuctions are included in the download.


### Get the code

The easiest way to get sources is to install Git on your machine and clone the repository.

`git clone https://github.com/EasyRF/contiki.git`

Alternatively you can download the sources from GitHub as a ZIP-file.

### Build the code

Because the CPU and platfrom drivers depend on ASF you need to 
