// Copyright (c) 2025 BT2C Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_VALIDATOR_H
#define BITCOIN_RPC_VALIDATOR_H

class CRPCTable;

/** Register validator RPC commands */
void RegisterValidatorRPCCommands(CRPCTable &t);

#endif // BITCOIN_RPC_VALIDATOR_H
