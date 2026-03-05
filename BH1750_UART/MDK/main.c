#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "stm32f10x_i2c.h"              // Keil::Device:StdPeriph Drivers:I2C
#include "stm32f10x_rcc.h"              // Keil::Device:StdPeriph Drivers:RCC
#include "stm32f10x_usart.h"            // Keil::Device:StdPeriph Drivers:USART
#include "stdio.h"

/* ================== MACRO CAM BIEN BH1750 ================== */
#define BH1750_ADDR 0x46
#define BH1750_PWR_ON 0x01
#define BH1750_RESET  0x07
#define BH1750_CONT_HRES_MODE 0x10

/* ================== KHAI BAO HAM ================== */
void delay_ms(uint16_t time);
void config_uart(void);
void USART1_SendChar(char c);
void USART1_SendString(char *s);
void Config_I2C(void);
void I2C_WriteByte(uint8_t address, uint8_t data);
void BH1750_Init(void);
uint16_t BH1750_ReadLight(void);

/* ================== HAM DELAY ================== */
void delay_ms(uint16_t time){ 
    SysTick->LOAD = 72000 - 1;      
    SysTick->VAL = 0;               
    SysTick->CTRL = 0x00000005;     
    
    for(uint16_t i = 0; i < time; i++){
        while(!(SysTick->CTRL & (1 << 16))); 
    }
    SysTick->CTRL = 0;              
}

/* ================== CAU HINH UART1 ================== */
void config_uart(void){
    GPIO_InitTypeDef gpio_pin;
    USART_InitTypeDef usart_init;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    // TX - PA9
    gpio_pin.GPIO_Pin = GPIO_Pin_9; 
    gpio_pin.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_pin.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_pin);
    
    // RX - PA10
    gpio_pin.GPIO_Pin = GPIO_Pin_10; 
    gpio_pin.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_pin.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_pin);

    usart_init.USART_BaudRate = 115200;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_StopBits = USART_StopBits_1;
    
    USART_Init(USART1, &usart_init); 
    USART_Cmd(USART1, ENABLE); 
}   
    
void USART1_SendChar(char c) {
    USART_SendData(USART1, c);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void USART1_SendString(char *s) {
    while (*s) {
        USART1_SendChar(*s++);
    }
}

/* ================== CAU HINH I2C1 ================== */
void Config_I2C(void){
    GPIO_InitTypeDef GPIO_init;
    I2C_InitTypeDef I2C_init;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    GPIO_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_init.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_init);
    
    I2C_init.I2C_ClockSpeed = 100000;
    I2C_init.I2C_Mode = I2C_Mode_I2C;
    I2C_init.I2C_DutyCycle = I2C_DutyCycle_2; 
    I2C_init.I2C_OwnAddress1 = 0x00; 
    I2C_init.I2C_Ack = I2C_Ack_Enable; 
    I2C_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    I2C_Init(I2C1, &I2C_init);
    
    I2C_Cmd(I2C1, ENABLE);
}

void I2C_WriteByte(uint8_t address, uint8_t data){
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); 

    I2C_Send7bitAddress(I2C1, address, I2C_Direction_Transmitter); 
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); 

    I2C_SendData(I2C1, data); 
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); 
    
    I2C_GenerateSTOP(I2C1, ENABLE);
}

/* ================== XU LY CAM BIEN BH1750 ================== */
void BH1750_Init(void) {
    delay_ms(10);
    I2C_WriteByte(BH1750_ADDR, BH1750_PWR_ON); 
    delay_ms(10);
    I2C_WriteByte(BH1750_ADDR, BH1750_CONT_HRES_MODE); 
    delay_ms(200); 
}

/* LOGIC DOC I2C CHUAN CUA ONG */
uint16_t BH1750_ReadLight() {
    uint16_t value = 0;
    uint8_t lsb;
    uint8_t msb;
    
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, BH1750_ADDR, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    // Doc msb
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    msb = I2C_ReceiveData(I2C1);

    // Doc lsb
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    lsb = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    value = ((msb << 8) | lsb) / 1.2;
    return value;
}

/* ================== CHUONG TRINH CHINH ================== */
int main(void){
    uint16_t lux_value;
    char buffer[50];
    
    Config_I2C();
    config_uart();
    
    USART1_SendString("System is preparing... \r\n");
    BH1750_Init();
    USART1_SendString("BH1750 Ready! \r\n");
    delay_ms(200);

    while(1){
        lux_value = BH1750_ReadLight();       
        
        sprintf(buffer, "Value LUX: %u \r\n", lux_value);
        USART1_SendString(buffer);
        
        delay_ms(500);      
    }
}