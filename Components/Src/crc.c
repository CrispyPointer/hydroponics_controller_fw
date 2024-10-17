/**************************************************
 * @file crc.c
 * @date 3-5-2016
 * @author JWA, PL
 * @brief CRC 16 calculation
 * @copyright (c) Copyright Cleantron 2016
 * @ingroup CRC
 *
 ****************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "crc.h"
#include "define.h"
#include <string.h>

/* Uncommenting this line uses a RAM table instead of a ROM table.
 * This can also be used to generate the ROM table (when the polynomial is
 * modified or something like that).
 */
//#define USE_RAM_INSTEAD_OF_ROM_TABLE

#define CRC16_POLYNOM      0xA001u
#define CRC16_TABLE_LENGTH 256u

/**
 * @brief Struct to store a parameter for hardware CRC peripheral
 */
typedef struct
{
    CRC_HandleTypeDef* handler;
} HW_CRC_PARAMS;

static HW_CRC_PARAMS hw_crc;

#if defined(USE_RAM_INSTEAD_OF_ROM_TABLE)
STATIC uint16_t crc16_table[CRC16_TABLE_LENGTH]; /* 512 bytes of RAM */
#else
static const uint16_t crc16_table[CRC16_TABLE_LENGTH] = /* 512 bytes of ROM */ {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, //
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, //
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, //
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, //
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, //
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, //
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, //
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, //
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, //
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, //
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, //
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, //
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, //
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, //
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, //
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, //
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, //
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, //
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, //
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, //
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, //
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, //
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, //
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, //
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, //
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, //
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, //
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, //
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, //
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, //
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, //
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040  //
};
#endif /* USE_RAM_INSTEAD_OF_ROM_TABLE */

/**
 * @brief Check CRC table and crc_calc() with known answer
 * test CRC with known answer www.lammertbies.nl/comm/info/nl_crc-calculation.html
 * @return true if CRC table is correct and calculation is correct otherwise false
 */
static bool crc_check_table(void)
{
    static const char test_str[] = "123456789";
    bool retval = true;
    const uint16_t crc_str = crc_calc(test_str, sizeof(test_str) - 1u);
    if (crc_str != 0xBB3Du)
    {
#ifndef DISABLE_CONSOLE
        printf_("CRC table not correct %04X\r\n", crc_str);
#endif // !DISABLE_CONSOLE
        retval = false;
    }
#if !defined(USE_RAM_INSTEAD_OF_ROM_TABLE)
    const uint16_t crc_tbl = crc_calc(crc16_table, sizeof(crc16_table));
    if (crc_tbl != 0x7205u)
    {
#ifndef DISABLE_CONSOLE
        printf_("CRC ROM table damaged %04X\r\n", crc_tbl);
#endif // !DISABLE_CONSOLE
        retval = false;
    }
#endif
    return retval;
}

void crc_init(CRC_HandleTypeDef* hcrc)
{
    hw_crc.handler = hcrc;

#if defined(USE_RAM_INSTEAD_OF_ROM_TABLE)
    for (uint16_t i = 0u; i < CRC16_TABLE_LENGTH; i++)
    {
        uint16_t crc = 0;
        uint16_t c = i;
        for (uint16_t j = 0u; j < 8u; j++)
        {
            crc = (uint16_t)((crc >> 1) ^ (-((crc ^ c) & 1u) & CRC16_POLYNOM));
            c >>= 1;
        }
        crc16_table[i] = crc; /* print 'crc' for generating ROM table */
    }
#endif /* USE_RAM_INSTEAD_OF_ROM_TABLE */

    (void)crc_check_table();
}

/* Note that this function is called about 2500 to 10000+ times per second.
 * It's has great effects on the overall performance.
 */
uint16_t crc_calc(const void* buf, const uint32_t length)
{
    const uint8_t* const src = (const uint8_t*)buf; //lint !e9079

    uint16_t crc = 0;
    for (uint32_t i = 0u; i < length; ++i)
    {
        crc = (uint16_t)((crc >> 8) ^ crc16_table[(uint8_t)(crc ^ src[i])]);
    }
    return crc;
}

uint16_t crc_hw_calc(const void* buf, const uint32_t length, const uint16_t polynom)
{
#ifdef STM32F030xx
    UNUSED(polynom);
    uint16_t crc = (uint16_t)HAL_CRC_Calculate(hw_crc.handler, (uint32_t*)buf, length); //lint !e9079 !e9005
#else                                                                                   // STM32F030xx
    uint16_t crc = 0;
    HAL_StatusTypeDef ret;
    ret = HAL_CRCEx_Polynomial_Set(hw_crc.handler, polynom, CRC_POLYLENGTH_16B);
    if (ret != HAL_OK)
    {
        printf_("HW CRC-16 configuration failed.\r\n");
    }
    else
    {
        crc = (uint16_t)HAL_CRC_Calculate(hw_crc.handler, (uint32_t*)buf, length); //lint !e9079 !e9005
    }
#endif

    return crc;
}
