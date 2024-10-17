/**
 * @file authentication.c
 * @brief Authentication module
 *
 * @copyright Copyright (c) Nekoco 2023
 */
#include "authentication.h"

#include "timer.h"

void auth_renew_values(CERTIFY* const certify)
{
    certify->z += timer_get_uptime() & 0xFu;
    uint32_t value = 0u;
    timer_reset_module_timer(&value);
    certify->z += value;
}

bool auth_unlock(CERTIFY* const certify, const uint32_t key)
{
    certify->z = (36967u * (certify->z & 0xFFFFu)) + (certify->z >> 16u) + 1u;
    certify->w = (18001u * (certify->w & 0xFFFFu)) + (certify->w >> 16u) + 1u;
    const uint32_t value = (certify->z << 16u) + certify->w;
    return key == value;
}
