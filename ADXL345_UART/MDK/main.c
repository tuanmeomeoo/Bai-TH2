#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include <stdio.h>
//cs-pa4
//scl-pa5
//sda-pa7
//sdo-pa6
/* ================== DINH NGHIA MACRO ADXL345 ================== */
#define ADXL345_DEVID        0x00
#define ADXL345_POWER_CTL    0x2D
#define ADXL345_DATA_FORMAT  0x31
#define ADXL345_DATAX0       0x32

#define CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

/* ================== KHAI BAO HAM ================== */
void delay_ms(uint16_t time);
void UART_Config(void);
void UART_SendChar(char c);
void UART_SendString(char *s);
void SPI_Config(void);
uint8_t SPI_Transfer(uint8_t data);
void ADXL345_Write(uint8_t reg, uint8_t data);
uint8_t ADXL345_ReadReg(uint8_t reg);
void ADXL345_Init(void);
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

/* ================== HAM DELAY CHUAN SYSTICK ================== */
void delay_ms(uint16_t time){ 
    SysTick->LOAD = 72000 - 1;      
    SysTick->VAL = 0;               
    SysTick->CTRL = 0x00000005;     
    
    for(uint16_t i = 0; i < time; i++){
        while(!(SysTick->CTRL & (1 << 16))); 
    }
    SysTick->CTRL = 0;              
}

/* ================== CAU HINH UART ================== */
void UART_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // TX - PA9
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = 115200;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendChar(char c)
{
    USART_SendData(USART1, (uint16_t)c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendString(char *s)
{
    while(*s) UART_SendChar(*s++);
}

/* ================== CAU HINH SPI1 (MODE 3) ================== */
void SPI_Config(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    // SCK (PA5) + MOSI (PA7)
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // MISO (PA6)
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    // CS (PA4)
    gpio.GPIO_Pin = GPIO_Pin_4;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    
    // Mac dinh keo CS len cao (Khong giao tiep)
    CS_HIGH();

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;

    // Bat buoc chon Mode 3 cho ADXL345
    spi.SPI_CPOL = SPI_CPOL_High;
    spi.SPI_CPHA = SPI_CPHA_2Edge;

    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

/* ================== TRUYEN NHAN SPI ================== */
uint8_t SPI_Transfer(uint8_t data)
{
    // Cho bo dem truyen rong
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);

    // Cho nhan du lieu phan hoi tu Slave
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

/* ================== CAC HAM XU LY ADXL345 ================== */
void ADXL345_Write(uint8_t reg, uint8_t data)
{
    CS_LOW();
    SPI_Transfer(reg);       // Gui dia chi thanh ghi can ghi
    SPI_Transfer(data);      // Gui du lieu
    CS_HIGH();
}

uint8_t ADXL345_ReadReg(uint8_t reg)
{
    uint8_t value;
    CS_LOW();
    SPI_Transfer(0x80 | reg); // Bit 7 len 1 de bao che do Doc
    value = SPI_Transfer(0x00); // Gui 1 byte rac de keo du lieu ve
    CS_HIGH();
    return value;
}

void ADXL345_Init(void)
{
    ADXL345_Write(ADXL345_DATA_FORMAT, 0x0B); // Do phan giai cao, dai do +/- 16g
    ADXL345_Write(ADXL345_POWER_CTL, 0x08);   // Bat che do do luong (Measurement Mode)
}

void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buff[6];
    
    CS_LOW();

    // 0xC0 = Bit 7 (Read) + Bit 6 (Multi-byte)
    SPI_Transfer(0xC0 | ADXL345_DATAX0);

    // Doc lien tuc 6 byte (X0, X1, Y0, Y1, Z0, Z1)
    for(int i = 0; i < 6; i++){
        buff[i] = SPI_Transfer(0x00);
    }

    CS_HIGH();

    // Ghep LSB va MSB an toan
    *x = (int16_t)((buff[1] << 8) | buff[0]);
    *y = (int16_t)((buff[3] << 8) | buff[2]);
    *z = (int16_t)((buff[5] << 8) | buff[4]);
}

/* ================== CHUONG TRINH CHINH ================== */
int main(void)
{
    int16_t x, y, z;
    uint8_t id;
    char buffer[100];

    SystemInit();
    UART_Config();
    SPI_Config();

    // Kiem tra ket noi voi cam bien
    id = ADXL345_ReadReg(ADXL345_DEVID);
    sprintf(buffer, "Device ID: %d \r\n", id);
    UART_SendString(buffer);

    ADXL345_Init();
    UART_SendString("ADXL345 Ready! \r\n");

    while(1)
    {
        ADXL345_ReadXYZ(&x, &y, &z);

        sprintf(buffer, "X: %d | Y: %d | Z: %d \r\n", x, y, z);
        UART_SendString(buffer);

        delay_ms(500);
    }
}