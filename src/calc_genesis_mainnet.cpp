#include <iostream>
#include <primitives/block.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <uint256.h>
#include <util/strencodings.h>

// Forward declaration of CreateGenesisBlock function
CBlock CreateGenesisBlock(uint32_t nTimeTx, uint32_t nTimeBlock, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward);

int main() {
    // Use the same parameters as mainnet in chainparams.cpp
    CBlock genesis = CreateGenesisBlock(1345083810, 1345084287, 2179302059u, 0x1d00ffff, 1, 50 * COIN);
    
    std::cout << "BT2C Mainnet Genesis Block:" << std::endl;
    std::cout << "Genesis Hash: " << genesis.GetHash().ToString() << std::endl;
    std::cout << "Merkle Root: " << genesis.hashMerkleRoot.ToString() << std::endl;
    
    return 0;
}
