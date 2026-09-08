/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "drv_uart.h"
#include "VoiceControl.h"
#include "hal_data.h"
#include "Servor.h"

static void drv_uart_write_blocking(uint8_t const * p_data, uint32_t length);
static uint8_t drv_uart_read_blocking(void);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
static volatile int g_uart7_tx_complete = 0;
static volatile int g_uart7_rx_complete = 0;

 volatile uint8_t g_uart2_rx_ready = 0;//正在接收
 volatile uint8_t g_uart2_rx_in_progress = 0;//正在处理

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

fsp_err_t drv_uart_init(void)
{
    fsp_err_t err;

    /* 打开串口 */
    err = g_uart7.p_api->open(g_uart7.p_ctrl, g_uart7.p_cfg);
    if(FSP_SUCCESS != err)
        {__BKPT();
          return err;
        }

  //打开串口2 两种打开串口方式
    err = R_SCI_UART_Open(&g_uart2_ctrl, &g_uart2_cfg);

    R_SCI_UART_Read(&g_uart2_ctrl, (uint8_t *)&g_uart2_rx_byte, 1U);
    if(FSP_SUCCESS != err)
        {
        __BKPT();
            return err;
        }

    return FSP_SUCCESS;
}

void drv_uart_wait_for_tx(void)
{
    while (!g_uart7_tx_complete); // 阻塞等待
    g_uart7_tx_complete = 0;
}

void drv_uart_wait_for_rx(void)
{
    while (!g_uart7_rx_complete); // 阻塞等待
    g_uart7_rx_complete = 0;
}


void uart7_callback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_TX_DATA_EMPTY://发送的数据为空情况  tx发送的缓冲区为空 说明发送完成
        {
            g_uart7_tx_complete  = 1;//将完成置为1   表示现在可以接收其他数据
            break;
        }
        case UART_EVENT_TX_COMPLETE://这个指的是发送这个整体操作
        {
            g_uart7_tx_complete  = 1;
            break;
        }
        case UART_EVENT_RX_COMPLETE://完成一个字节的接收触发这个
        {
            g_uart7_rx_complete = 1;
            break;
        }
        default:
        {
            break;
        }
    }
}

void uart2_callback(uart_callback_args_t * p_args)//该语音串口只是负责接收 （板子接收）所以只有rx
{
    if (UART_EVENT_RX_COMPLETE == p_args->event)//当板子收到消息
    {
        voice_uart2_rx_byte(g_uart2_rx_byte);
        (void) R_SCI_UART_Read(&g_uart2_ctrl, (uint8_t *) &g_uart2_rx_byte, 1U);
    }
}


int _write(int fd, char *pBuffer, int size)
{
    (void) fd;

    if ((NULL == pBuffer) || (size <= 0))
    {
        return 0;
    }

    for (int i = 0; i < size; i++)
    {
        uint8_t c = (uint8_t) pBuffer[i];
        if ('\n' == c)
        {
            uint8_t cr = '\r';
            drv_uart_write_blocking(&cr, 1U);
        }
        drv_uart_write_blocking(&c, 1U);
    }

    return size;
}

int _read(int fd, char *pBuffer, int size)
{
    (void) fd;

    if ((NULL == pBuffer) || (size <= 0))
    {
        return 0;
    }

    uint8_t ch = drv_uart_read_blocking();

    if ('\r' == ch)
    {
        uint8_t crlf[2] = { '\r', '\n' };
        drv_uart_write_blocking(crlf, 2U);
        pBuffer[0] = '\n';
    }
    else
    {
        drv_uart_write_blocking(&ch, 1U);
        pBuffer[0] = (char) ch;
    }

    return 1;
}


/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

static void drv_uart_write_blocking(uint8_t const * p_data, uint32_t length)
{
    fsp_err_t err;
    volatile uint32_t timeout;

    if ((NULL == p_data) || (0U == length))
    {
        return;
    }

    while (1)
    {
        g_uart7_tx_complete = 0;
        err = g_uart7.p_api->write(g_uart7.p_ctrl, (uint8_t const * const) p_data, length);

        if (FSP_SUCCESS == err)
        {
            break;
        }

        if (FSP_ERR_IN_USE != err)
        {
            return;
        }

        timeout = 1000000U;
        while ((0 == g_uart7_tx_complete) && (timeout--))
        {
            ;
        }
        if (0U == timeout)
        {
            return;
        }
    }

    timeout = 10000000U;
    while ((0 == g_uart7_tx_complete) && (timeout--))
    {
        ;
    }
    g_uart7_tx_complete = 0;
}

static uint8_t drv_uart_read_blocking(void)
{
    fsp_err_t err;
    uint8_t ch = 0;

    while (1)
    {
        g_uart7_rx_complete = 0;
        err = g_uart7.p_api->read(g_uart7.p_ctrl, &ch, 1U);
        if (FSP_SUCCESS == err)
        {
            break;
        }
        if (FSP_ERR_IN_USE != err)
        {
            return 0;
        }
    }

    while (0 == g_uart7_rx_complete)
    {
        ;
    }
    g_uart7_rx_complete = 0;
    return ch;
}



