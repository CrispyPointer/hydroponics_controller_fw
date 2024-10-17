/**
 * @file authentication.h
 * @brief Authentication module
 *
 * @copyright Copyright (c) Cleantron 2023
 */
#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @struct CERTIFY
 * @brief Container random authentication values
 */
typedef struct
{
    uint32_t w;
    uint32_t z;
} CERTIFY;

/**
 * Initialise internal structure 'CERTIFY.z', using time.
 * @param certify Reference to CERTIFY structure
 */
void auth_renew_values(CERTIFY* const certify) __attribute__((__nonnull__(1)));

/**
 * Calculate random authentication values
 * @param certify Reference to CERTIFY structure
 * @param key Potential key to unlock the system
 * @return True the key is valid the system can be unlocked
 */
bool auth_unlock(CERTIFY* const certify, const uint32_t key) __attribute__((__nonnull__(1)));

#endif /* AUTHENTICATION_H */
