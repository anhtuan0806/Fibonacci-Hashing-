#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <cstdint>
#include <string>
#include <functional>

// Check if a number is prime
static bool isPrime(size_t n) {
    if (n < 2) return false;
    for (size_t i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// Return the next prime number >= n
static size_t nextPrime(size_t n) {
    while (!isPrime(n)) ++n;
    return n;
}

// Simple open addressing hash table for integer keys using linear probing
class HashTable {
public:
    using HashFunc = std::function<size_t(int, size_t)>;

    // Construct table with given capacity and hashing function
    HashTable(size_t capacity, HashFunc func)
        : keys(capacity), states(capacity, State::Empty), sz(0), hashFunc(func) {}

    // Remove all entries
    ~HashTable() { clear(); }

    // Insert a key using linear probing with automatic resizing
    void insert(int key) {
        insertInternal(key);
        if (loadFactor() > 0.7) {
            rehash(nextPrime(keys.size() * 2));
        }
    }

    // Check if key is present
    bool contains(int key) const {
        size_t idx = hashFunc(key, keys.size());
        size_t start = idx;
        while (states[idx] != State::Empty) {
            if (states[idx] == State::Filled && keys[idx] == key)
                return true;
            idx = (idx + 1) % keys.size();
            if (idx == start) break;
        }
        return false;
    }

    // Remove a key if present
    bool remove(int key) {
        size_t idx = hashFunc(key, keys.size());
        size_t start = idx;
        while (states[idx] != State::Empty) {
            if (states[idx] == State::Filled && keys[idx] == key) {
                states[idx] = State::Deleted;
                --sz;
                return true;
            }
            idx = (idx + 1) % keys.size();
            if (idx == start) break;
        }
        return false;
    }

    // Current load factor
    double loadFactor() const { return static_cast<double>(sz) / keys.size(); }

    // Average chain length is 1 with open addressing
    double averageChainLength() const { return sz > 0 ? 1.0 : 0.0; }

    // Maximum chain length is also 1
    size_t maxChainLength() const { return sz > 0 ? 1 : 0; }

    // Estimated memory usage in bytes
    size_t memoryUsage() const {
        return keys.size() * (sizeof(int) + sizeof(State));
    }

    // Clear the table
    void clear() {
        std::fill(states.begin(), states.end(), State::Empty);
        sz = 0;
    }

private:
    void insertInternal(int key) {
        size_t idx = hashFunc(key, keys.size());
        while (states[idx] == State::Filled) {
            if (keys[idx] == key)
                return; // already in table
            idx = (idx + 1) % keys.size();
        }
        keys[idx] = key;
        states[idx] = State::Filled;
        ++sz;
    }

    void rehash(size_t newCapacity) {
        std::vector<int> oldKeys = keys;
        std::vector<State> oldStates = states;
        keys.assign(newCapacity, 0);
        states.assign(newCapacity, State::Empty);
        sz = 0;
        for (size_t i = 0; i < oldKeys.size(); ++i) {
            if (oldStates[i] == State::Filled)
                insertInternal(oldKeys[i]);
        }
    }

    enum class State { Empty, Filled, Deleted };
    std::vector<int> keys;
    std::vector<State> states;
    size_t sz;
    HashFunc hashFunc;
};

// Fibonacci hashing for integers
static size_t fibonacciHash(int key, size_t tableSize) {
    static const uint32_t fib = 2654435769u; // 2^32 / golden ratio
    return (static_cast<uint64_t>(static_cast<uint32_t>(key) * fib)) % tableSize;
}

// Simple modulo hashing
static size_t moduloHash(int key, size_t tableSize) {
    return static_cast<uint32_t>(key) % tableSize;
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

// Benchmark the table using the provided keys
Metrics runTest(const std::vector<int>& keys, HashTable::HashFunc func,
                size_t initialSize) {
    HashTable table(initialSize, func);

    auto start = std::chrono::high_resolution_clock::now();
    for (int k : keys) table.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    double insertTime =
        std::chrono::duration<double, std::milli>(end - start).count();

    double loadFactor = table.loadFactor();
    double avgChain = table.averageChainLength();
    size_t maxChain = table.maxChainLength();
    size_t mem = table.memoryUsage();

    start = std::chrono::high_resolution_clock::now();
    for (int k : keys) table.contains(k);
    end = std::chrono::high_resolution_clock::now();
    double findTime =
        std::chrono::duration<double, std::milli>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (int k : keys) table.remove(k);
    end = std::chrono::high_resolution_clock::now();
    double eraseTime =
        std::chrono::duration<double, std::milli>(end - start).count();

    return {loadFactor, avgChain, maxChain, insertTime, findTime, eraseTime, mem};
}

// Pretty-print metrics to stdout
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
    const size_t tableSize = 17; // initial prime size

    size_t numKeys = 0;
    std::cout << "Enter number of keys: ";
    if (!(std::cin >> numKeys) || numKeys == 0) {
        std::cerr << "Invalid number of keys" << std::endl;
        return 1;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1u << 30);

    std::vector<int> randomKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i) randomKeys[i] = dist(rng);

    std::vector<int> sequentialKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i) sequentialKeys[i] = static_cast<int>(i);

    std::vector<int> clusteredKeys(numKeys);
    for (size_t i = 0; i < numKeys; ++i) {
        int base = static_cast<int>((i / 10) * 20);
        clusteredKeys[i] = base + static_cast<int>(i % 10);
    }

    struct Dataset {
        std::string name;
        std::vector<int>* data;
    } datasets[] = {{"Random", &randomKeys},
                    {"Sequential", &sequentialKeys},
                    {"Clustered", &clusteredKeys}};

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
