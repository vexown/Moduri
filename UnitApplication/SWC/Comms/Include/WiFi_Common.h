#ifndef WIFI_COMMON_H
#define WIFI_COMMON_H

/* Server defines */
#define PC_IP_ADDRESS       "192.168.1.194"  //My PC's IP address assigned by the Tenda WiFi router DHCP server (may change)
#define PICO_W_IP_ADDRESS   "192.168.1.188"  //Pico W's IP address assigned by the Tenda WiFi router DHCP server (may change)
#define SERVER_PORT         (uint16_t)12345  //TODO - server port for testing, later you can think about which one to use permanently

/* Misc defines */
#define E_OK                0
#define ERRNO_FAIL         -1
#define NO_FLAG             0


#endif
