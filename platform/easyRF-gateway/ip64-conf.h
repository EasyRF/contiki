#ifndef IP64CONF_H
#define IP64CONF_H

#include "ip64-eth-interface.h"

#define IP64_CONF_UIP_FALLBACK_INTERFACE ip64_eth_interface
#define IP64_CONF_INPUT                  ip64_eth_interface_input

#define IP64_CONF_DHCP                   1

#include "enc28j60-ip64-driver.h"

#define IP64_CONF_ETH_DRIVER             enc28j60_ip64_driver

#endif // IP64CONF_H