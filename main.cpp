#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <cstdint>
#include <string>
#include <functional>
#include <fstream>

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

    // Average cluster length (contiguous filled slots)
    double averageChainLength() const {
        size_t clusterCount = 0;
        size_t total = 0;
        for (size_t i = 0; i < keys.size();) {
            if (states[i] == State::Filled) {
                size_t len = 0;
                while (i < keys.size() && states[i] == State::Filled) {
                    ++len;
                    ++i;
                }
                ++clusterCount;
                total += len;
            } else {
                ++i;
            }
        }
        if (clusterCount == 0) return 0.0;
        return static_cast<double>(total) / clusterCount;
    }

    // Maximum cluster length
    size_t maxChainLength() const {
        size_t maxLen = 0;
        for (size_t i = 0; i < keys.size();) {
            if (states[i] == State::Filled) {
                size_t len = 0;
                while (i < keys.size() && states[i] == State::Filled) {
                    ++len;
                    ++i;
                }
                if (len > maxLen) maxLen = len;
            } else {
                ++i;
            }
        }
        return maxLen;
    }

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

// Write a CSV row with metrics
void writeCsv(std::ofstream& out, const std::string& dataset,
              const std::string& method, const Metrics& m) {
    out << dataset << ',' << method << ',' << m.loadFactor << ',' << m.avgChain
        << ',' << m.maxChain << ',' << m.insertTime << ',' << m.findTime << ','
        << m.eraseTime << ',' << m.memory << '\n';
}

// Benchmark the table using the provided keys
Metrics runTest(const std::vector<int>& keys, HashTable::HashFunc func,
                size_t initialSize, size_t runs = 3) {
    double totalInsert = 0.0;
    double totalFind = 0.0;
    double totalErase = 0.0;

    double loadFactor = 0.0;
    double avgChain = 0.0;
    size_t maxChain = 0;
    size_t mem = 0;

    for (size_t i = 0; i < runs; ++i) {
        HashTable table(initialSize, func);

        auto start = std::chrono::high_resolution_clock::now();
        for (int k : keys) table.insert(k);
        auto end = std::chrono::high_resolution_clock::now();
        totalInsert +=
            std::chrono::duration<double, std::micro>(end - start).count();

        if (i == 0) {
            loadFactor = table.loadFactor();
            avgChain = table.averageChainLength();
            maxChain = table.maxChainLength();
            mem = table.memoryUsage();
        }

        start = std::chrono::high_resolution_clock::now();
        for (int k : keys) table.contains(k);
        end = std::chrono::high_resolution_clock::now();
        totalFind +=
            std::chrono::duration<double, std::micro>(end - start).count();

        start = std::chrono::high_resolution_clock::now();
        for (int k : keys) table.remove(k);
        end = std::chrono::high_resolution_clock::now();
        totalErase +=
            std::chrono::duration<double, std::micro>(end - start).count();
    }

    return {loadFactor,         avgChain,
            maxChain,           totalInsert / runs,
            totalFind / runs,   totalErase / runs,
            mem};
}

// Pretty-print metrics to stdout
void printMetrics(const std::string& title, const Metrics& m) {
    std::cout << title << "\n";
    std::cout << "  Load factor       : " << m.loadFactor << "\n";
    std::cout << "  Avg chain length  : " << m.avgChain << "\n";
    std::cout << "  Max chain length  : " << m.maxChain << "\n";
    std::cout << "  Insert time (\xCE\xBCs)  : " << m.insertTime << "\n";
    std::cout << "  Find time (\xCE\xBCs)    : " << m.findTime << "\n";
    std::cout << "  Erase time (\xCE\xBCs)   : " << m.eraseTime << "\n";
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

    std::ofstream csv("results.csv");
    if (!csv) {
        std::cerr << "Failed to open results.csv" << std::endl;
        return 1;
    }
    csv << "Dataset,Method,LoadFactor,AverageCluster,MaxCluster,InsertTime(us),";
    csv << "FindTime(us),EraseTime(us),Memory(B)\n";

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
        writeCsv(csv, ds.name, "Fibonacci", fib);
        std::cout << "-- Modulo Hashing --\n";
        printMetrics("", mod);
        writeCsv(csv, ds.name, "Modulo", mod);
        std::cout << std::endl;
    }

    return 0;
}
