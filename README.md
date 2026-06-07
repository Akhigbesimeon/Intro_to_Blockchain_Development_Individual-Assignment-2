# Blockchain-Based Attendance & Token Economy System

An advanced, cryptographically secure attendance tracking application implemented in C. Building upon the foundational immutable ledger of Formative 1, this iteration introduces a fully functional decentralized token economy. The system demonstrates advanced blockchain concepts including mempool management, dual state-ledger models (UTXO and Account-based), and Proof-of-Work (PoW) consensus simulations.

# System Overview
The system bridges academic tracking with economic incentives. Administrators mark student attendance, which triggers the generation of a token reward transaction (e.g., 10 coins for PRESENT). Instead of being written immediately to the chain, these transactions are held in a Pending Pool. The network state is only updated after simulated miners successfully perform Proof-of-Work to confirm the blocks.

## Key Features
* **Immutable Ledger:** Records cannot be altered once appended to the chain.
* **Cryptographic Hashing:** Uses OpenSSL's SHA-256 for deterministic block hashing.
* **Simulated Digital Signatures:** Generates a deterministic 72-byte buffer mimicking an ECDSA footprint to authenticate entries.
* **File-Based Persistence:** Loads valid student credentials from a CSV-formatted `students.txt` file at startup.
* **Tamper Detection Engine:** Iterates through the linked list in memory to validate `hash` and `previous_hash` integrity, catching unauthorized modifications.

## Prerequisites

To compile and run this application, your system must have the following installed:
* **GCC (GNU Compiler Collection):** To compile the C source code.
* **OpenSSL Development Library:** Required for the cryptographic hashing functions.
  * Ubuntu/Debian: `sudo apt-get install libssl-dev`
  * CentOS/RHEL: `sudo yum install openssl-devel`
  * macOS (Homebrew): `brew install openssl`

## Compilation and Setup
* Clone or download this repository to your local machine.

* Ensure that your registry file, students.txt, is located in the same directory as the source code.

* Compile the source code. Note: You must now link both the ssl and crypto libraries. Run the following command in your terminal:
`gcc Student_attendance.c -o student_attendance -lcrypto`

## Running the Program
Execute the compiled program: `./student_attendance`

## Switching Between Transaction Models
You will be prompted to select a Ledger Model:

* **Enter 1 for UTXO Model:** Balances are calculated dynamically by summing unspent transaction outputs.

* **Enter 2 for Account Model:** Balances are maintained as static integers tied to a student ID, protected by transaction nonces.

## Setting the Mining Difficulty Level
You will be prompted to set the PoW difficulty (e.g., enter 2).

* This integer defines how many leading zeros are required in the SHA-256 hash for a block to be considered valid.

* **Note:** Higher numbers (3 or 4) will significantly increase the computational time required to confirm blocks.

## How to Test Mining Simulations
To observe the end-to-end flow of transactions and consensus, follow these steps within the CLI interface:

* **Stage Transactions:** Select Option 1 (Mark Attendance). Input a valid Student ID and status. The system will calculate the token reward and place the unconfirmed block into the Pending Pool. You can repeat this step to stage multiple blocks.

* **Execute Consensus:** Select Option 2 (Mine Pending Pool). The system will prompt you to choose a mining simulation:

* **Solo Mining (Option 1):** Watch the system iterate through the nonce until the PoW difficulty is met, rewarding the single miner.

* **Pool Mining (Option 2):** Simulates distributed hash power. The system will output a table detailing the proportional distribution of block rewards among three distinct miners, minus a 2% pool fee.

* **Cloud Mining (Option 3):** Simulates rented computational power. Input a rental duration (1 to 5 rounds) to view a breakdown of upfront costs versus randomized mining rewards, including an unprofitability warning if net profit is negative.

* **Verify State Updates:** Select Option 3 (View Balances & Ledger). Depending on your chosen Transaction Model at startup, the system will output the newly confirmed token balances for the student registry.



