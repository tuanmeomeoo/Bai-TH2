#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include <stdio.h>

/* ================== DHT11 CONFIG ================== */
#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_Pin_1

/* ================== DELAY ================== */
void TIM2_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim;
    tim.TIM_Prescaler = 72 - 1;      // 72MHz / 72 = 1MHz (1us)
    tim.TIM_Period = 0xFFFF;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

void Delay_us(uint16_t us)
{
    TIM_SetCounter(TIM2, 0);
    while (TIM_GetCounter(TIM2) < us);
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

/* ================== DHT11 ================== */
void DHT_Pin_Output(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT_PORT, &gpio);
}

void DHT_Pin_Input(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT_PORT, &gpio);
}

uint8_t DHT_Start(void)
{
    DHT_Pin_Output();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(18);              // 18ms

    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(30);

    DHT_Pin_Input();

    Delay_us(40);
    if (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) return 1;

    Delay_us(80);
    if (!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) return 1;

    Delay_us(80);
    return 0;
}

uint8_t DHT_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0;
    uint8_t bit[8];

    for (i = 0; i < 8; i++)
    {
        /* Ð?i DHT kéo HIGH b?t d?u bit */
        while (!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN));

        Delay_us(40);

        /* N?u sau 40us mà v?n HIGH ? bit = 1 */
        if (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN))
        {
            bit[i] = 1;
            while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN));
        }
        else
        {
            bit[i] = 0;
        }
    }

    /* Ghép 8 bit thành 1 byte */
    data = bit[0]*128 +
           bit[1]*64  +
           bit[2]*32  +
           bit[3]*16  +
           bit[4]*8   +
           bit[5]*4   +
           bit[6]*2   +
           bit[7];

    return data;
}


uint8_t DHT_Read(uint8_t *temp, uint8_t *humi)
{
    uint8_t rh_int, rh_dec, t_int, t_dec, checksum;

    if (DHT_Start()) return 1;

    rh_int   = DHT_ReadByte();
    rh_dec   = DHT_ReadByte();
    t_int    = DHT_ReadByte();
    t_dec    = DHT_ReadByte();
    checksum = DHT_ReadByte();

    if ((rh_int + rh_dec + t_int + t_dec) != checksum) return 2;

    *temp = t_int;
    *humi = rh_int;
    return 0;
}

/* ================== UART ================== */
void USART1_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef us;
    us.USART_BaudRate = 9600;
    us.USART_WordLength = USART_WordLength_8b;
    us.USART_StopBits = USART_StopBits_1;
    us.USART_Parity = USART_Parity_No;
    us.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    us.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &us);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendString(char *str)
{
    while (*str)
    {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

/* ================== MAIN ================== */
int main(void)
{
    uint8_t temp, humi;
    char buf[50];

    SystemInit();
    TIM2_Init();
    USART1_Init();

    while (1)
    {
        if (DHT_Read(&temp, &humi) == 0)
        {
            sprintf(buf, "Temp: %d C | Humi: %d %%\r\n", temp, humi);
            UART_SendString(buf);
        }
        else
        {
            UART_SendString("DHT11 ERROR\r\n");
        }

        Delay_ms(1000);    // 1 giây
    }
}