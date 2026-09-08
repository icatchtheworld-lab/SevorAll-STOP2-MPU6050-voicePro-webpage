/*
 * Wifi.h
 *
 *  Created on: 2026年3月15日
 *      Author: 36315
 */

#ifndef DRIVERS_WIFI_H_
#define DRIVERS_WIFI_H_

#include <stdint.h>

typedef struct
{
    uint32_t server_socket;//服务器套接字
    uint32_t client_socket;//客户端套接字
} wifi_tcp_server_t;

int Wifi_Init(void);

int Wifi_Reset(void);

int Wifi_ConnectSTA(char const * ssid, char const * password);

int Wifi_QueryIpSta(void);

int Wifi_TcpServerStart(uint16_t port, wifi_tcp_server_t * out_server);

int Wifi_TcpServerPollClient(wifi_tcp_server_t * server);

int Wifi_TcpRecv(uint32_t socket_id, uint8_t * buf, uint32_t buf_size, uint32_t timeout_ms);

int Wifi_TcpSend(uint32_t socket_id, uint8_t const * data, uint32_t length);

#endif /* DRIVERS_WIFI_H_ */
