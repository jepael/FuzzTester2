/*
 * LcdDisplay.cpp
 *
 *  Created on: Jan 30, 2020
 *      Author: miche
 */

#include <Framework/LcdDisplay.h>
#include <Framework/SysTickSubscribers.h>

#include "stm32f1xx_hal.h"

// #define LCD_ADDR (0x27 << 1)

#define PIN_RS    (1 << 0)
#define PIN_EN    (1 << 2)
#define BACKLIGHT (1 << 3)

#define LCD_DELAY_MS 5



LcdDisplay::LcdDisplay(I2C_HandleTypeDef* hI2c, uint8_t i2cChannel,
      UPDATE_LCD_FUNCTION_PTR callbackFunction, uint16_t refreshTime, uint8_t sysTickSubscriberIndex)
: _hI2c(hI2c),
  _i2cChannel(i2cChannel << 1),
  _callbackFunction(callbackFunction)
{
   SysTickSubscribers::SetSubscriber(sysTickSubscriberIndex, this);
   SysTickSubscribers::SetInterval(sysTickSubscriberIndex, refreshTime);
}


LcdDisplay::~LcdDisplay()
{
}


void LcdDisplay::SetLine(uint8_t line, const char* text)
{
	// set address to 0x40 for second line;
   SendCommand(0b10000000 + 0x40 * line);
   SendString(text);
}


void LcdDisplay::I2C_Scan()
{
    //char info[] = "Scanning I2C bus...\r\n";
    //HAL_UART_Transmit(&huart2, (uint8_t*)info, strlen(info), HAL_MAX_DELAY);

    HAL_StatusTypeDef res;
    for (uint8_t i = 0; i < 128; i++) {
        res = HAL_I2C_IsDeviceReady(_hI2c, i << 1, 1, 10);
        if(res == HAL_OK)
        {
        	while(true) {}; // Set a breakpoint here to check the channel
        }
    }

    //HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
}


void LcdDisplay::Init()
{
    // Wait 15..40 ms after LCD power applied
    HAL_Delay(50);

    // Try several times - one try may go just for initializing the
    // IO Expander to get LCD bus to known state.
    for (uint8_t i = 0; i < 5; i++)
    {
        // Try going to 8-bit mode using two nibbles of 0x03
        SendCommand(0b00110011);
        // Wait more than 4.1 ms
        HAL_Delay(6);
    }

    // Try going to 4-bit mode using nibbles 0x03 and 0x02
    // First nibble 0x03 is 8-bit command to go to 8-bit mode
    // Second nibble 0x02 is 8-bit command to go to 4-bit mode
    SendCommand(0b00110010);
    // Should be in 4-bit mode now, but without correct line/font info
    // So, resend Function Set command in 4-bit mode as two nibbles

    // 4-bit mode, 2 lines, 5x7 format
    SendCommand(0b00101000);
    // Entry Mode Set (increase cursor, no display shift)
    SendCommand(0b00000110);
    // display & cursor home (keep this!)
    SendCommand(0b00000010);
    // Cursor Home needs more than 1.5 ms to execute
    HAL_Delay(3);
    // display on, cursor off, blink off
    SendCommand(0b00001100);
    // clear display (optional here)
    SendCommand(0b00000001);
    // Clear Display needs more than 1.5 ms to execute
    HAL_Delay(3);

}


HAL_StatusTypeDef LcdDisplay::SendInternal(uint8_t data, uint8_t flags)
{
    HAL_StatusTypeDef res;
    for(;;) {
        res = HAL_I2C_IsDeviceReady(_hI2c, _i2cChannel, 1, HAL_MAX_DELAY);
        if(res == HAL_OK)
            break;
    }

    uint8_t up = data & 0xF0;
    uint8_t lo = (data << 4) & 0xF0;

    uint8_t data_arr[4];
    data_arr[0] = up|flags|BACKLIGHT|PIN_EN;
    data_arr[1] = up|flags|BACKLIGHT;
    data_arr[2] = lo|flags|BACKLIGHT|PIN_EN;
    data_arr[3] = lo|flags|BACKLIGHT;

    //res = HAL_I2C_Master_Transmit_IT(_hI2c, _i2cChannel, data_arr, sizeof(data_arr));
    res = HAL_I2C_Master_Transmit(_hI2c, _i2cChannel, data_arr, sizeof(data_arr), HAL_MAX_DELAY);
    //HAL_Delay(LCD_DELAY_MS);
    return res;
}


void LcdDisplay::SendCommand(uint8_t cmd)
{
    SendInternal(cmd, 0);
}


void LcdDisplay::SendData(uint8_t data)
{
    SendInternal(data, PIN_RS);
}


void LcdDisplay::SendString(const char *str)
{
    while(*str)
    {
        SendData((uint8_t)(*str));
        str++;
    }
}


void LcdDisplay::OnTick()
{

   (*_callbackFunction)();
}
