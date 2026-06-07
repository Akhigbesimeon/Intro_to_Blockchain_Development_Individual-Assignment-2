#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <openssl/sha.h>

#define MAX_STUDENTS 100
#define MAX_PENDING 50
#define HASH_HEX_LEN 65
#define SIG_LEN 72

// System Configuration
int MINING_DIFFICULTY = 2;
int TX_MODEL = 2;

// Core data structures
typedef struct {
    char student_id[20];
    char full_name[50];
    char course_code[10];
} Student;

// Updated block with reward, TX ID, and nonce
typedef struct Block {
    int index;
    time_t timestamp;
    char student_id[20];
    char full_name[50];
    char course_code[10];
    char status[10];
    int token_reward;
    char transaction_id[HASH_HEX_LEN];
    int nonce;
    char previous_hash[HASH_HEX_LEN];
    unsigned char signature[SIG_LEN];
    char hash[HASH_HEX_LEN];
    struct Block* next;
} Block;

// Account-based structures
typedef struct TxNode {
    char sender[20];
    char recipient[20];
    int amount;
    int fee;
    int nonce;
    struct TxNode* next;
} TxNode;

typedef struct {
    char student_id[20];
    int balance;
    int tx_nonce;
    TxNode* history;
} Account;

// UTXO-based structures
typedef struct UTXO {
    char tx_id[HASH_HEX_LEN];
    char owner_id[20];
    int amount;
    int is_spent;
    struct UTXO* next;
} UTXO;

// Global variables
Student registry[MAX_STUDENTS];
int student_count = 0;

Block* blockchain_head = NULL;
Block* blockchain_tail = NULL;

Block* pending_pool[MAX_PENDING];
int pending_count = 0;

Account accounts[MAX_STUDENTS];
UTXO* utxo_set = NULL;

// Helper functions
void compute_hash(const char* input, char* output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input, strlen(input), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[HASH_HEX_LEN - 1] = '\0';
}

void calculate_block_hash(Block* b, char* out_hash) {
    char buffer[1024];
    // Added token_reward, transaction_id, and nonce to the hash calculation
    snprintf(buffer, sizeof(buffer), "%d%ld%s%s%s%s%d%s%d%s",
             b->index, b->timestamp, b->student_id, b->full_name,
             b->course_code, b->status, b->token_reward,
             b->transaction_id, b->nonce, b->previous_hash);
    compute_hash(buffer, out_hash);
}

void generate_mock_signature(Block* b) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "SIG_%s_%ld", b->student_id, b->timestamp);
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)buffer, strlen(buffer), temp_hash);
    memset(b->signature, 0, SIG_LEN);
    memcpy(b->signature, temp_hash, SHA256_DIGEST_LENGTH);
    memcpy(b->signature + SHA256_DIGEST_LENGTH, temp_hash, SHA256_DIGEST_LENGTH);
}

// Core requirements
void load_registry() {
    FILE* file = fopen("students.txt", "r");
    if (!file) {
        printf("ERROR: students.txt is missing.\n");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file) && student_count < MAX_STUDENTS) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        char* id = strtok(line, ",");
        char* name = strtok(NULL, ",");
        char* course = strtok(NULL, ",");

        if (id && name && course) {
            strncpy(registry[student_count].student_id, id, 19);
            strncpy(registry[student_count].full_name, name, 49);
            strncpy(registry[student_count].course_code, course, 9);
            student_count++;
        }
    }
    fclose(file);

    if (student_count == 0) {
        printf("ERROR: students.txt is empty.\n");
        exit(1);
    }

    // Initialize accounts for the ledger
    for(int i=0; i<student_count; i++) {
        strcpy(accounts[i].student_id, registry[i].student_id);
        accounts[i].balance = 0;
        accounts[i].tx_nonce = 0;
        accounts[i].history = NULL;
    }

    printf("Loaded %d students from registry.\n", student_count);
}

void create_genesis_block() {
    Block* genesis = (Block*)malloc(sizeof(Block));
    genesis->index = 0;
    genesis->timestamp = time(NULL);
    strcpy(genesis->student_id, "GENESIS");
    strcpy(genesis->full_name, "SYSTEM");
    strcpy(genesis->course_code, "NONE");
    strcpy(genesis->status, "SYSTEM");
    genesis->token_reward = 0;
    strcpy(genesis->transaction_id, "00000000000000000000000000000000");
    genesis->nonce = 0;
    memset(genesis->previous_hash, '0', 64);
    genesis->previous_hash[64] = '\0';

    generate_mock_signature(genesis);
    calculate_block_hash(genesis, genesis->hash);

    genesis->next = NULL;
    blockchain_head = genesis;
    blockchain_tail = genesis;
    printf("Genesis block created.\n");
}

// Ledger & balance updates
void create_utxo(const char* owner, int amount, const char* tx_id) {
    UTXO* new_utxo = (UTXO*)malloc(sizeof(UTXO));
    strcpy(new_utxo->owner_id, owner);
    strcpy(new_utxo->tx_id, tx_id);
    new_utxo->amount = amount;
    new_utxo->is_spent = 0;
    new_utxo->next = utxo_set;
    utxo_set = new_utxo;
}

void process_block_reward(Block* b) {
    if (b->token_reward <= 0) return;

    if (TX_MODEL == 1) {
        create_utxo(b->student_id, b->token_reward, b->transaction_id);
    } else {
        for(int i=0; i<student_count; i++) {
            if(strcmp(accounts[i].student_id, b->student_id) == 0) {
                accounts[i].balance += b->token_reward;
                break;
            }
        }
    }
}

// Attendance & mempool
void mark_attendance() {
    if (pending_count >= MAX_PENDING) {
        printf("Pending pool full. Please mine blocks first.\n");
        return;
    }

    char id[20], status[10];
    printf("\nEnter Student ID: ");
    scanf("%19s", id);

    int found_idx = -1;
    for (int i = 0; i < student_count; i++) {
        if (strcmp(registry[i].student_id, id) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) {
        printf("ERROR: Student ID not found.\n");
        return;
    }

    printf("Enter Status (PRESENT/ABSENT/LATE): ");
    scanf("%9s", status);

    // Convert input to uppercase
    for (int i = 0; status[i]; i++) {
        status[i] = toupper((unsigned char)status[i]);
    }

    Block* new_block = (Block*)malloc(sizeof(Block));
    new_block->index = blockchain_tail->index + pending_count + 1;
    new_block->timestamp = time(NULL);
    strcpy(new_block->student_id, registry[found_idx].student_id);
    strcpy(new_block->full_name, registry[found_idx].full_name);
    strcpy(new_block->course_code, registry[found_idx].course_code);
    strcpy(new_block->status, status);

    // Reward logic
    if(strcmp(status, "PRESENT") == 0) new_block->token_reward = 10;
    else if(strcmp(status, "LATE") == 0) new_block->token_reward = 5;
    else new_block->token_reward = 0;

    // Generate transaction hash
    char tx_buffer[256];
    snprintf(tx_buffer, sizeof(tx_buffer), "TX_%s_%d_%ld", new_block->student_id, new_block->token_reward, new_block->timestamp);
    compute_hash(tx_buffer, new_block->transaction_id);

    new_block->nonce = 0;
    pending_pool[pending_count] = new_block;
    pending_count++;

    printf("Attendance added to PENDING POOL for %s. Reward: %d coins.\n", new_block->full_name, new_block->token_reward);
}

// Mining functionality
int check_proof_of_work(char* hash) {
    for (int i = 0; i < MINING_DIFFICULTY; i++) {
        if (hash[i] != '0') return 0;
    }
    return 1;
}

void add_block_to_chain(Block* b) {
    strcpy(b->previous_hash, blockchain_tail->hash);
    generate_mock_signature(b);

    // Proof of Work loop
    int attempts = 0;
    do {
        b->nonce++;
        attempts++;
        calculate_block_hash(b, b->hash);
    } while (!check_proof_of_work(b->hash));

    b->next = NULL;
    blockchain_tail->next = b;
    blockchain_tail = b;

    process_block_reward(b);
    printf("Mined Block %d | Nonce: %d | Attempts: %d\n", b->index, b->nonce, attempts);
}

void mine_solo() {
    printf("\n-- Starting Solo Mining --\n");
    for (int i = 0; i < pending_count; i++) {
        add_block_to_chain(pending_pool[i]);
    }
    printf("Solo Mining complete. Added %d blocks to chain.\n", pending_count);
    pending_count = 0;
}

void mine_pool() {
    printf("\n-- Starting Pool Mining Simulation --\n");
    int miners[3] = {1000, 2500, 1500};
    int total_hash = 5000;
    int pool_reward = 50 * pending_count;
    float fee_deduction = pool_reward * 0.02; // 2% pool fee
    float net_reward = pool_reward - fee_deduction;

    for (int i = 0; i < pending_count; i++) {
        add_block_to_chain(pending_pool[i]);
    }

    printf("\nPool Mining Rewards Distributed (Total Pending: %d blocks):\n", pending_count);
    printf("----------------------------------------------------\n");
    printf("Miner ID | Hash Rate | Share %% | Reward (Coins)\n");
    for(int i=0; i<3; i++) {
        float share = ((float)miners[i] / total_hash);
        printf("Miner_%d  | %d      | %.2f%%  | %.2f\n", i+1, miners[i], share * 100.0, share * net_reward);
    }
    printf("----------------------------------------------------\n");
    printf("Pool Mining complete. Mined %d blocks.\n", pending_count);
    pending_count = 0;
}

void mine_cloud() {
    int rounds;
    printf("\nEnter rental duration (1 to 5 rounds): ");
    scanf("%d", &rounds);

    int cost_per_round = 15;
    int total_cost = rounds * cost_per_round;
    int estimated_reward = (rand() % 20 + 5) * rounds;

    printf("\n-- Cloud Mining Simulation --\n");
    printf("Rounds Rented: %d\n", rounds);
    printf("Total Rental Fee: %d coins\n", total_cost);
    printf("Gross Mining Reward: %d coins\n", estimated_reward);

    int net = estimated_reward - total_cost;
    printf("Net Profit: %d coins\n", net);

    if (net < 0) {
        printf("WARNING: Cloud rental was unprofitable this session.\n");
    }

    // Process blocks via cloud power
    for (int i = 0; i < pending_count; i++) {
        add_block_to_chain(pending_pool[i]);
    }
    printf("Cloud Mining complete. Mined %d blocks.\n", pending_count);
    pending_count = 0;
}

void mine_pending_pool() {
    if (pending_count == 0) {
        printf("Pending pool is empty. Nothing to mine.\n");
        return;
    }
    int choice;
    printf("\nSelect Mining Method:\n");
    printf("1. Solo Mining\n2. Pool Mining\n3. Cloud Mining\nOption: ");
    scanf("%d", &choice);

    if(choice == 1) mine_solo();
    else if(choice == 2) mine_pool();
    else if(choice == 3) mine_cloud();
    else printf("Invalid option.\n");
}

// View state
void view_balances() {
    printf("\n-- Token Balances (%s Model) --\n", TX_MODEL == 1 ? "UTXO" : "Account");
    if (TX_MODEL == 1) {
        // Calculate UTXO balances dynamically
        int temp_bal[MAX_STUDENTS] = {0};
        UTXO* current = utxo_set;
        while(current) {
            if(!current->is_spent) {
                for(int i=0; i<student_count; i++) {
                    if(strcmp(registry[i].student_id, current->owner_id) == 0) {
                        temp_bal[i] += current->amount;
                    }
                }
            }
            current = current->next;
        }
        for(int i=0; i<student_count; i++) {
            printf("%s (%s): %d coins\n", registry[i].full_name, registry[i].student_id, temp_bal[i]);
        }
    } else {
        for(int i=0; i<student_count; i++) {
            printf("%s (%s): %d coins (Nonce: %d)\n", registry[i].full_name, accounts[i].student_id, accounts[i].balance, accounts[i].tx_nonce);
        }
    }
}

// Main function
int main() {
    printf("Select Ledger Model (1: UTXO, 2: Account): ");
    scanf("%d", &TX_MODEL);
    printf("Set Mining Difficulty (e.g., 2): ");
    scanf("%d", &MINING_DIFFICULTY);

    load_registry();
    create_genesis_block();

    int choice;
    do {
        printf("\n-- Student Attendance & Token System --\n");
        printf("1. Mark Attendance (Adds to Mempool)\n");
        printf("2. Mine Pending Pool\n");
        printf("3. View Balances & Ledger\n");
        printf("4. Exit\n");
        printf("Select option: ");
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: mark_attendance(); break;
            case 2: mine_pending_pool(); break;
            case 3: view_balances(); break;
            case 4: printf("Exiting...\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 4);

    return 0;
}
