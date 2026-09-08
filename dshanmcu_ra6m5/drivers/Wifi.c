/*
 * Wifi.c
 *
 *  Created on: 2026年3月15日
 *      Author: 36315
 */


#include "Wifi.h"

#include "hal_data.h"

#include <stdbool.h>
#include <stdint.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    uint8_t * p_buf;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
} ringbuf_t;//环形缓冲区的结构体

static uint8_t g_uart6_rb_storage[1024];//实际存放数据的仓库
static ringbuf_t g_uart6_rb;//创建一个结构体

static volatile bool g_uart6_opened = false;
static volatile bool g_uart6_tx_done = false;//uart6发送完成

static uint8_t g_uart6_rx_byte;//uart6接收的一个数据

static char g_w800_line_buf[256];//用于累积接收到的字符
static char g_w800_cmd_buf[128];//用于存储命令

#define W800_LINE_BUF_SIZE   (sizeof(g_w800_line_buf))
#define W800_CMD_BUF_SIZE    (sizeof(g_w800_cmd_buf))

enum
{
    ESUCCESS = 0
};

typedef enum
{
    STA = 0,
    SoftAP = 2,
    APSTA = 3
} WorkType;

typedef enum
{
    TCP = 0,
    UDP = 1
} NetworkProtocol;

typedef enum
{
    Client = 0,
    Server = 1
} LocalRole;

typedef struct
{
    NetworkProtocol Protocl;
    LocalRole Role;
    char const * IP;
    uint32_t RemotePort;
    uint32_t LocalPort;
    uint32_t SocketPort;
} ConnectInfo;//两层结构体

static void delay_ms(uint32_t ms);
static void ringbuf_init(ringbuf_t * rb, uint8_t * buf, uint32_t size);
static void ringbuf_flush(ringbuf_t * rb);
static bool ringbuf_push(ringbuf_t * rb, uint8_t b);
static int ringbuf_pop(ringbuf_t * rb);

static fsp_err_t uart6_write_blocking(uint8_t const * p, uint32_t n);
static int uart6_read_byte_nonblocking(void);
static fsp_err_t uart6_start_rx(void);
static int uart6_read_stream(uint8_t * buf, uint32_t length, uint32_t timeout_ms);
static int uart6_read_until_crlf2(char * buf, uint32_t buf_size, uint32_t timeout_ms);
static int wifi_read_ok_value(uint32_t * value, uint32_t timeout_ms);
static int WiFiBtDevCmdRet(char const * ret, uint32_t timeout_ms);

static int WiFiBtDevSetWorkType(WorkType type);
static int WiFiBtDevEnableDHCP(void);
static int WiFiBtDevSetSsid(char const * ssid);
static int WiFiBtDevSetKeyAscii(uint8_t index, char const * key);
static int WiFiBtDevJoin(void);
static int WiFiBtDevConnectWiFi(char const * name, char const * password);
static int WiFiBtDevConnect(ConnectInfo * info);
static int WiFiBtDevFindConnectedClientSocket(uint32_t server_socket_id, uint32_t * out_client_socket_id);
static int WiFiBtDevTcpRecv(uint32_t socket_id, uint8_t * buf, uint32_t buf_size, uint32_t timeout_ms);
static int WiFiBtDevTcpSend(uint32_t socket_id, uint8_t const * data, uint32_t length);

static bool g_wifi_raw_echo = false;//控制是否将 WiFi 模块返回的原始数据打印到调试控制台


static void delay_ms(uint32_t ms)
{
    (void) R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

static void ringbuf_init(ringbuf_t * rb, uint8_t * buf, uint32_t size) //rb：指向 ringbuf_t 结构体的指针
{
    rb->p_buf = buf;  //将传入缓冲区的数组，赋值给环形缓冲区控制结构体的p_buf成员
    rb->size = size;
    rb->head = 0U;
    rb->tail = 0U;
}

static void ringbuf_flush(ringbuf_t * rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}

static bool ringbuf_push(ringbuf_t * rb, uint8_t b)
{
    uint32_t next = rb->head + 1U;
    if (next >= rb->size)
    {
        next = 0U;
    }
    if (next == rb->tail)
    {
        return false;
    }
    rb->p_buf[rb->head] = b;
    rb->head = next;
    return true;
}
//将环形缓冲区的数据弹出
static int ringbuf_pop(ringbuf_t * rb)
{
    if (rb->tail == rb->head)
    {
        return -1;
    }
    uint8_t b = rb->p_buf[rb->tail];
    uint32_t next = rb->tail + 1U;
    if (next >= rb->size)
    {
        next = 0U;
    }
    rb->tail = next;
    return (int) b;
}
//向uart6发送at指令
static fsp_err_t uart6_write_blocking(uint8_t const * p, uint32_t n)//p是发送的指令  n是长度
{
    if (!g_uart6_opened)
    {
        return FSP_ERR_NOT_OPEN;
    }
    //先将发送完成设置为0
    g_uart6_tx_done = false;
    fsp_err_t err = g_uart6.p_api->write(g_uart6.p_ctrl, p, n);//将指令通过uart6发送给w800
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    while (!g_uart6_tx_done)
    {
        __NOP();
    }
    return FSP_SUCCESS;
}

static int uart6_read_byte_nonblocking(void)
{
    return ringbuf_pop(&g_uart6_rb);
}

static fsp_err_t uart6_start_rx(void)
{
    return g_uart6.p_api->read(g_uart6.p_ctrl, &g_uart6_rx_byte, 1U);
}

static int uart6_read_stream(uint8_t * buf, uint32_t length, uint32_t timeout_ms)
{
    if ((NULL == buf) || (0U == length))
    {
        return -EINVAL;
    }

    uint32_t got = 0U;
    for (uint32_t t = 0U; t < timeout_ms; t++)
    {
        while (got < length)
        {
            int ch = uart6_read_byte_nonblocking();
            if (ch < 0)
            {
                break;
            }
            buf[got++] = (uint8_t) ch;
        }
        if (got >= length)
        {
            return ESUCCESS;
        }
        delay_ms(1U);
    }

    return -EIO;
}

static int uart6_read_until_crlf2(char * buf, uint32_t buf_size, uint32_t timeout_ms)
{
    if ((NULL == buf) || (buf_size < 5U))
    {
        return -EINVAL;
    }

    buf[0] = '\0';
    uint32_t i = 0U;
    for (uint32_t t = 0U; t < timeout_ms; t++)
    {
        int ch = uart6_read_byte_nonblocking();// 从环形缓冲区取字节
        if (ch >= 0)
        {
            uint8_t b = (uint8_t) ch;
            if (g_wifi_raw_echo)
            {
                (void) putchar((int) b);
            }

            if (i < (buf_size - 1U))
            {
                buf[i++] = (char) b;//填充buf
                buf[i] = '\0';
            }

            if (strstr(buf, "\r\n\r\n") != NULL)
            {
                return (int) i;
            }
            if (strstr(buf, "+ERR") != NULL)
            {
                return -EIO;
            }
        }
        delay_ms(1U);
    }

    return -ETIMEDOUT;
}
//用于检验wifi返回回复是否匹配
static int wifi_read_ok_value(uint32_t * value, uint32_t timeout_ms)//最多循环timeout_ms次
{
    if (NULL == value)
    {
        return -EINVAL;
    }
    char hdr[96];
    int n = uart6_read_until_crlf2(hdr, sizeof(hdr), timeout_ms);//将环形缓冲区的数据（wifi回复at指令的内容）填充到hdr，并返回回复内容的长度
    if (n < 0)
    {
        return n;
    }
    //解析回复的指令   从+ok开始截断
    char * ok = strstr(hdr, "+OK=");
        if (ok != NULL)
    {
        *value = (uint32_t) strtoul(ok + 4, NULL, 10);
        return ESUCCESS;
    }

    ok = strstr(hdr, "+OK");
    if (ok != NULL)
    {
        *value = 0U;
        return ESUCCESS;
    }

    return -EIO;
}

static int WiFiBtDevCmdRet(char const * ret, uint32_t timeout_ms)//ret：期望接收信息
{
    if ((NULL == ret) || (!g_uart6_opened))
    {
        return -EINVAL;
    }

    char * buf = g_w800_line_buf;
    buf[0] = '\0';
    uint32_t i = 0U;
    for (uint32_t t = 0U; t < timeout_ms; t++)
    {
        int ch = uart6_read_byte_nonblocking();//弹出的数据
        if (ch >= 0)
        {
            if (i < (W800_LINE_BUF_SIZE - 1U))
            {
                buf[i++] = (char) (uint8_t) ch;
                buf[i] = '\0';
            }

            if (g_wifi_raw_echo)
            {
                (void) putchar(ch);
            }

            if (strstr(buf, ret) != NULL)
            {
                return ESUCCESS;
            }
            if (strstr(buf, "+ERR") != NULL)
            {
                return -EIO;
            }
        }

        delay_ms(1U);
    }

    return -ETIMEDOUT;
}

static int WiFiBtDevSetWorkType(WorkType type)
{
    char str[32];
    int n = snprintf(str, sizeof(str), "AT+WPRT=%d\r\n", (int) type);//用于将格式化的数据写入字符串，并保证不会超出指定的缓冲区大小
    //如果成功，返回应该写入的字符数
    if (n <= 0)
    {
        return -EIO;
    }

    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) str, (uint32_t) n))
    {
        return -EIO;
    }
    delay_ms(100U);
    return WiFiBtDevCmdRet("+OK", 3000U);
}

static int WiFiBtDevEnableDHCP(void)//当无线网卡作为STA时，该指令用于设置/查询本端IP地址
{
    char const * str = "AT+NIP=0\r\n";
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) str, (uint32_t) strlen(str)))
    {
        return -EIO;
    }
    delay_ms(100U);
    return WiFiBtDevCmdRet("+OK", 3000U);
}

static int WiFiBtDevSetSsid(char const * ssid)
{
    if (NULL == ssid)
    {
        return -EINVAL;
    }

    char str[96];
    int n = snprintf(str, sizeof(str), "AT+SSID=%s\r\n", ssid);
    if (n <= 0)
    {
        return -EIO;
    }

    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) str, (uint32_t) n))
    {
        return -EIO;
    }

    delay_ms(100U);
    return WiFiBtDevCmdRet("+OK", 3000U);
}

static int WiFiBtDevSetKeyAscii(uint8_t index, char const * key)
{
    if (NULL == key)
    {
        return -EINVAL;
    }

    char str[128];
    int n = snprintf(str, sizeof(str), "AT+KEY=1,%u,\"%s\"\r\n", (unsigned) index, key);
    if (n <= 0)
    {
        return -EIO;
    }

    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) str, (uint32_t) n))
    {
        return -EIO;
    }

    delay_ms(100U);
    return WiFiBtDevCmdRet("+OK", 3000U);
}

static int WiFiBtDevJoin(void)
{
    char const * str = "AT+WJOIN\r\n";
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) str, (uint32_t) strlen(str)))
    {
        return -EIO;
    }

    delay_ms(200U);
    return WiFiBtDevCmdRet("+OK", 10000U);
}

static int WiFiBtDevConnectWiFi(char const * name, char const * password)
{
    int ret = WiFiBtDevSetSsid(name);
    if (ESUCCESS != ret)
    {
        return ret;
    }

    ret = WiFiBtDevSetKeyAscii(0U, password);
    if (ESUCCESS != ret)
    {
        return ret;
    }

    ret = WiFiBtDevJoin();
    return ret;
}

static int WiFiBtDevConnect(ConnectInfo * info)
{
    if (NULL == info)
    {
        return -EINVAL;
    }

    char * cmd = g_w800_cmd_buf;
    int n = 0;
    if (Server == info->Role)
    {
        n = snprintf(cmd, W800_CMD_BUF_SIZE, "AT+SKCT=%u,1,120,0,%u\r\n",
                     (unsigned) info->Protocl,
                     (unsigned) info->LocalPort);
    }
    else
    {
        if (NULL == info->IP)
        {
            return -EINVAL;
        }
        n = snprintf(cmd, W800_CMD_BUF_SIZE, "AT+SKCT=%d,%d,%s,%u,%u\r",
                     (int) info->Protocl,
                     (int) info->Role,
                     info->IP,
                     (unsigned) info->RemotePort,
                     (unsigned) info->LocalPort);
    }

    if (n <= 0)
    {
        return -EIO;
    }

    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd, (uint32_t) n))
    {
        return -EIO;
    }
    delay_ms(100U);

    uint32_t socket_id = 0U;
    int ret = wifi_read_ok_value(&socket_id, 5000U);
    if (ESUCCESS != ret)
    {
        return ret;
    }

    info->SocketPort = socket_id;
    return ESUCCESS;
}

static int WiFiBtDevFindConnectedClientSocket(uint32_t server_socket_id, uint32_t * out_client_socket_id)
{
    //查找已经连接的客户
    if (NULL == out_client_socket_id)
    {
        return -EINVAL;
    }

    char cmd_local[32];
    int n = snprintf(cmd_local, sizeof(cmd_local), "AT+SKSTT=%u\r\n", (unsigned) server_socket_id);
    if (n <= 0)
    {
        return -EIO;
    }

    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd_local, (uint32_t) n))
    {
        return -EIO;
    }

    char * resp = g_w800_line_buf;
    int rn = uart6_read_until_crlf2(resp, (uint32_t) W800_LINE_BUF_SIZE, 1500U);
    if (rn < 0)
    {
        return rn;
    }

    char * p = strstr(resp, "+OK=");
    if (NULL == p)
    {
        return -EIO;
    }

    p += 4;
    while ((p != NULL) && (*p != '\0'))
    {
        while ((*p == '\r') || (*p == '\n'))
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }

        char * endptr = NULL;
        unsigned long sock = strtoul(p, &endptr, 10);
        if ((endptr == p) || (NULL == endptr) || (*endptr != ','))
        {
            break;
        }
        p = endptr + 1;

        unsigned long st = strtoul(p, &endptr, 10);
        if (endptr == p)
        {
            break;
        }

        if ((sock != (unsigned long) server_socket_id) && (2UL == st))
        {
            *out_client_socket_id = (uint32_t) sock;
            return ESUCCESS;
        }

        char * nl = strstr(endptr, "\r\n");
        if (NULL == nl)
        {
            break;
        }
        p = nl + 2;
    }

    return -EIO;
}

static int WiFiBtDevTcpRecv(uint32_t socket_id, uint8_t * buf, uint32_t buf_size, uint32_t timeout_ms)
{
    if ((NULL == buf) || (buf_size < 2U))
    {
        return -EINVAL;
    }
//构造AT命令
    char cmd[48];
    int n = snprintf(cmd, sizeof(cmd), "AT+SKRCV=%u,%u\r",
                     (unsigned) socket_id, // socket ID
                     (unsigned) (buf_size - 1U));//读取指定socket的接收缓冲区中的数据，完成后返回
    //
    if (n <= 0)
    {
        return -EIO;
    }

    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd, (uint32_t) n))//mpu给wifi发送指令依旧是通过uart6
    {
        return -EIO;
    }
//此时指令已经发送给w800  │  W800 内部处理:                                                         │
   //1. 解析命令: 从 socket 3 读取最多 63 字节                              │
   //2. 检查 socket 3 的接收缓冲区
    //3发现客户端发送了C=90   开始构造相应+OK=4\r\n\r\n"  (告诉 MCU 有 4 字节数据)
    //UART6 接收 "+OK=4\r\n\r\n"
    //WiFi 模块发送: "+OK=4\r\n\r\n"

    uint32_t size = 0U;
    int ret = wifi_read_ok_value(&size, timeout_ms);//传的是size的地址
    if (ESUCCESS != ret)
    {
        if (-ETIMEDOUT == ret)
        {
            return 0;
        }
        return ret;
    }

    if (0U == size)
    {
        return 0;
    }

    if (size > (buf_size - 1U))
    {
        size = buf_size - 1U;
    }

    if (uart6_read_stream(buf, size, 1000U) != ESUCCESS)
    {
        return -EIO;
    }
    buf[size] = (uint8_t) '\0';
    return (int) size;
}

static int WiFiBtDevTcpSend(uint32_t socket_id, uint8_t const * data, uint32_t length)
{
    if ((NULL == data) || (0U == length))
    {
        return -EINVAL;
    }

    char cmd[48];
    int n = snprintf(cmd, sizeof(cmd), "AT+SKSND=%u,%u\r", (unsigned) socket_id, (unsigned) length);
    if (n <= 0)
    {
        return -EIO;
    }

    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd, (uint32_t) n))
    {
        return -EIO;
    }

    uint32_t actual = 0U;
    if (wifi_read_ok_value(&actual, 3000U) != ESUCCESS)
    {
        return -EIO;
    }
    if (actual < length)
    {
        length = actual;
    }

    if (FSP_SUCCESS != uart6_write_blocking(data, length))
    {
        return -EIO;
    }
    return ESUCCESS;
}


int Wifi_Init(void)
{
    if (g_uart6_opened)//查看uart6是否被打开过  打开直接返回 避免重复初始化
    {
        return ESUCCESS;
    }

    fsp_err_t err = g_uart6.p_api->open(g_uart6.p_ctrl, g_uart6.p_cfg);
    if (FSP_SUCCESS != err)
    {
        return -EIO;
    }

    ringbuf_init(&g_uart6_rb, g_uart6_rb_storage, sizeof(g_uart6_rb_storage));//初始化环形缓冲区
    g_uart6_opened = true;
    (void) uart6_start_rx();

    delay_ms(200U);
    ringbuf_flush(&g_uart6_rb);//清空缓冲区（实质上是覆盖）

    return ESUCCESS;
}

int Wifi_Reset(void)
{
    if (!g_uart6_opened)
    {
        return -EINVAL;
    }

    char const * cmd = "AT+Z\r";

    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd, (uint32_t) strlen(cmd)))
    {
        return -EIO;
    }

    uint32_t dummy = 0U;
    int ret = wifi_read_ok_value(&dummy, 3000U);

    delay_ms(2000U);
    ringbuf_flush(&g_uart6_rb);
    return ret;
}

int Wifi_ConnectSTA(char const * ssid, char const * password)
{
    if ((!g_uart6_opened) || (NULL == ssid) || (NULL == password))
    {
        return -EINVAL;
    }

    int ret = WiFiBtDevSetWorkType(STA);
    if (ESUCCESS != ret)
    {
        return ret;
    }

    ret = WiFiBtDevEnableDHCP();//查询ip地址等信息
    if (ESUCCESS != ret)
    {
        return ret;
    }

    ret = WiFiBtDevConnectWiFi(ssid, password);
    return ret;
}

int Wifi_QueryIpSta(void)
{
    if (!g_uart6_opened)
    {
        return -EINVAL;
    }

    char const * cmd = "AT+LKSTT\r";
    ringbuf_flush(&g_uart6_rb);
    if (FSP_SUCCESS != uart6_write_blocking((uint8_t const *) cmd, (uint32_t) strlen(cmd)))
    {
        return -EIO;
    }

    char * hdr = g_w800_line_buf;
    int n = uart6_read_until_crlf2(hdr, (uint32_t) W800_LINE_BUF_SIZE, 3000U);
    if (n < 0)
    {
        return n;
    }

    char * ok = strstr(hdr, "+OK=");
    if (NULL == ok)
    {
        return -EIO;
    }

    printf("LKSTT: %s\r\n", ok);
    return ESUCCESS;
}

int Wifi_TcpServerStart(uint16_t port, wifi_tcp_server_t * out_server)
{
    if ((!g_uart6_opened) || (NULL == out_server))
    {
        return -EINVAL;
    }

    ConnectInfo server =
    {
        .Protocl = TCP,
        .Role = Server,
        .IP = "0.0.0.0",
        .RemotePort = 0U,
        .LocalPort = (uint32_t) port,
        .SocketPort = 0U,
    };

    int ret = WiFiBtDevConnect(&server);
    if (ESUCCESS != ret)
    {
        return ret;
    }

    out_server->server_socket = server.SocketPort;
    out_server->client_socket = 0U;
    return ESUCCESS;
}

int Wifi_TcpServerPollClient(wifi_tcp_server_t * server)
{
    if ((!g_uart6_opened) || (NULL == server) || (0U == server->server_socket))
    {
        return -EINVAL;
    }

    uint32_t client = 0U;
    int ret = WiFiBtDevFindConnectedClientSocket(server->server_socket, &client);
    if (ESUCCESS == ret)
    {
        server->client_socket = client;
    }
    return ret;
}

int Wifi_TcpRecv(uint32_t socket_id, uint8_t * buf, uint32_t buf_size, uint32_t timeout_ms)
{
    if (!g_uart6_opened)
    {
        return -EINVAL;
    }
    return WiFiBtDevTcpRecv(socket_id, buf, buf_size, timeout_ms);
}

int Wifi_TcpSend(uint32_t socket_id, uint8_t const * data, uint32_t length)
{
    if (!g_uart6_opened)
    {
        return -EINVAL;
    }
    return WiFiBtDevTcpSend(socket_id, data, length);
}


void uart6_callback(uart_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }
//发送at指令向uart6
    if (UART_EVENT_RX_COMPLETE == p_args->event)//触发条件  是uart6接收完成（一个一个发一个一个收  ）
    {
        (void) ringbuf_push(&g_uart6_rb, g_uart6_rx_byte);//将数据压入环形缓冲区
        (void) uart6_start_rx();
    }
    //uart6缓冲区空了或者发送发送完成完成了  将tx置为1
    else if ((UART_EVENT_TX_COMPLETE == p_args->event) || (UART_EVENT_TX_DATA_EMPTY == p_args->event))

    {
        g_uart6_tx_done = true;
    }
    else
    {
    }
}

