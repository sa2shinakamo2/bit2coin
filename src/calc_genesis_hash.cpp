#include <iostream>
#include "kernel/chainparams.cpp"
#include "primitives/block.h"
#include "hash.h"

int main() {
    // Create genesis block with BT2C parameters
    CBlock genesis = CreateGenesisBlock(1345083810, 1345084287, 2179302059u, 0x1d00ffff, 1, 50 * COIN);
    
    // Calculate and print the hash
    uint256 hash = genesis.GetHash();
    std::cout << "Genesis Block Hash: " << hash.ToString() << std::endl;
    std::cout << "Genesis Merkle Root: " << genesis.hashMerkleRoot.ToString() << std::endl;
    
    return 0;
}
