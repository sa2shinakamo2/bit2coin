// Copyright (c) 2023-2025 The BT2C developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BT2C_VALIDATOR_STATUS_H
#define BT2C_VALIDATOR_STATUS_H

#include <iostream>

/** 
 * Validator status enum
 * Represents the current status of a validator in the network
 */
enum class ValidatorStatus {
    INACTIVE,       // Not currently staking
    ACTIVE,         // Actively staking and participating in consensus
    SLASHED,        // Has been slashed for malicious behavior
    PENDING_EXIT    // In the process of unstaking
};

// Add ostream operator for ValidatorStatus
inline std::ostream& operator<<(std::ostream& os, const ValidatorStatus& status) {
    switch (status) {
        case ValidatorStatus::INACTIVE:
            os << "INACTIVE";
            break;
        case ValidatorStatus::ACTIVE:
            os << "ACTIVE";
            break;
        case ValidatorStatus::SLASHED:
            os << "SLASHED";
            break;
        case ValidatorStatus::PENDING_EXIT:
            os << "PENDING_EXIT";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

#endif // BT2C_VALIDATOR_STATUS_H
