#ifndef WIFI_COMMON_H
#define WIFI_COMMON_H

/* Server defines */
#define PC_IP_ADDRESS               "192.168.1.194"  //My PC's IP address assigned by the Tenda WiFi router DHCP server (may change)
#define PICO_W_STATIC_IP_ADDRESS    "192.168.1.50"   //Static IP address for the Pico W
#define NETMASK_ADDR                "255.255.255.0"  //Subnet mask for the network (meaning you can use 192.168.1.1 to 192.168.1.254 for devices 
                                                     //(the 0 address is the network address and 255 is the broadcast address).
#define GATEWAY_ADDR                "192.168.1.1"    //Gateway IP address (in this case the Tenda Wifi router is the gateway so it's its' address)
#define SERVER_PORT                 (uint16_t)12345  //TODO - server port for testing, later you can think about which one to use permanently
#define TCP_PORT                    4242

/* Misc defines */
#define E_OK           0
#define ERRNO_FAIL     -1
#define NO_FLAG        0


#endif
