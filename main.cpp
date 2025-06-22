#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <cstdint>
#include <string>
#include <functional>

struct Node {
    uint32_t key;
    Node* next;
};

class HashTable {
public:
    using HashFunc = std::function<size_t(uint32_t, size_t)>;

    HashTable(size_t capacity, HashFunc func)
        : table(capacity, nullptr), sz(0), hashFunc(func) {}

    ~HashTable() { clear(); }

    void insert(uint32_t key) {
        size_t idx = hashFunc(key, table.size());
        Node* node = new Node{key, table[idx]};
        table[idx] = node;
        ++sz;
    }

    bool contains(uint32_t key) const {
        size_t idx = hashFunc(key, table.size());
        Node* cur = table[idx];
        while (cur) {
            if (cur->key == key) return true;
            cur = cur->next;
        }
        return false;
    }

    bool remove(uint32_t key) {
        size_t idx = hashFunc(key, table.size());
        Node* cur = table[idx];
        Node* prev = nullptr;
        while (cur) {
            if (cur->key == key) {
                if (prev)
                    prev->next = cur->next;
                else
                    table[idx] = cur->next;
                delete cur;
                --sz;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    double loadFactor() const { return static_cast<double>(sz) / table.size(); }

    double averageChainLength() const {
        size_t nonEmpty = 0;
        size_t total = 0;
        for (Node* head : table) {
            if (!head) continue;
            ++nonEmpty;
            Node* cur = head;
            while (cur) {
                ++total;
                cur = cur->next;
            }
        }
        if (nonEmpty == 0) return 0.0;
        return static_cast<double>(total) / nonEmpty;
    }

    size_t maxChainLength() const {
        size_t maxLen = 0;
        for (Node* head : table) {
            size_t len = 0;
            Node* cur = head;
            while (cur) {
                ++len;
                cur = cur->next;
            }
            if (len > maxLen) maxLen = len;
        }
        return maxLen;
    }

    size_t memoryUsage() const {
        return table.size() * sizeof(Node*) + sz * sizeof(Node);
    }

    void clear() {
        for (Node*& head : table) {
            Node* cur = head;
            while (cur) {
                Node* tmp = cur->next;
                delete cur;
                cur = tmp;
            }
            head = nullptr;
        }
        sz = 0;
    }

private:
    std::vector<Node*> table;
    size_t sz;
    HashFunc hashFunc;
};

static size_t fibonacciHash(uint32_t key, size_t tableSize) {
    static const uint32_t fib = 2654435769u; // 2^32 / golden ratio
    unsigned int shift = 32 - static_cast<unsigned int>(std::log2(tableSize));
    return (key * fib) >> shift;
}

static size_t moduloHash(uint32_t key, size_t tableSize) {
    return key % tableSize;
}

struct Metrics {
    double loadFactor;
    double avgChain;
    size_t maxChain;
    double insertTime;
    double findTime;
    double eraseTime;
    size_t memory;
};

Metrics runTest(const std::vector<uint32_t>& keys, HashTable::HashFunc func, size_t tableSize) {
    HashTable table(tableSize, func);

    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t k : keys) table.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    double insertTime = std::chrono::duration<double, std::milli>(end - start).count();

    double loadFactor = table.loadFactor();
    double avgChain = table.averageChainLength();
    size_t maxChain = table.maxChainLength();
    size_t mem = table.memoryUsage();

    start = std::chrono::high_resolution_clock::now();
    for (uint32_t k : keys) table.contains(k);
    end = std::chrono::high_resolution_clock::now();
    double findTime = std::chrono::duration<double, std::milli>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (uint32_t k : keys) table.remove(k);
    end = std::chrono::high_resolution_clock::now();
    double eraseTime = std::chrono::duration<double, std::milli>(end - start).count();

    return {loadFactor, avgChain, maxChain, insertTime, findTime, eraseTime, mem};
}

void printMetrics(const std::string& title, const Metrics& m) {
    std::cout << title << "\n";
    std::cout << "  Load factor       : " << m.loadFactor << "\n";
    std::cout << "  Avg chain length  : " << m.avgChain << "\n";
    std::cout << "  Max chain length  : " << m.maxChain << "\n";
    std::cout << "  Insert time (ms)  : " << m.insertTime << "\n";
    std::cout << "  Find time (ms)    : " << m.findTime << "\n";
    std::cout << "  Erase time (ms)   : " << m.eraseTime << "\n";
    std::cout << "  Memory usage (B)  : " << m.memory << "\n";
}

int main() {
    const size_t tableSize = 8192; // power of two

    size_t numKeys = 0;
    std::cout << "Enter number of keys: ";
    if (!(std::cin >> numKeys) || numKeys == 0) {
        std::cerr << "Invalid number of keys" << std::endl;
        return 1;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist(0, 1u << 30);

    std::vector<uint32_t> randomKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i)
        randomKeys[i] = dist(rng);

    std::vector<uint32_t> sequentialKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i)
        sequentialKeys[i] = static_cast<uint32_t>(i);

    std::vector<uint32_t> clusteredKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i) {
        uint32_t base = static_cast<uint32_t>((i / 10) * 20);
        clusteredKeys[i] = base + static_cast<uint32_t>(i % 10);
    }

    struct Dataset { std::string name; std::vector<uint32_t>* data; } datasets[] = {
        {"Random", &randomKeys},
        {"Sequential", &sequentialKeys},
        {"Clustered", &clusteredKeys}
    };

    for (const auto& ds : datasets) {
        std::cout << "===== Dataset: " << ds.name << " =====\n";
        Metrics fib = runTest(*ds.data, fibonacciHash, tableSize);
        Metrics mod = runTest(*ds.data, moduloHash, tableSize);
        std::cout << "-- Fibonacci Hashing --\n";
        printMetrics("", fib);
        std::cout << "-- Modulo Hashing --\n";
        printMetrics("", mod);
        std::cout << std::endl;
    }

    return 0;
}

