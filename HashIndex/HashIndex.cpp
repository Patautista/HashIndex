#include <iostream>
#include <vector>
#include <string>

// Classe que representa um bucket
class Bucket {
public:
    int local_depth;
    std::vector<int> records;

    Bucket(int depth) : local_depth(depth) {}

    bool isFull() {
        return records.size() >= 3;  // Capacidade máxima do bucket é 3
    }

    bool isEmpty() {
        return records.empty();
    }
};

// Classe que representa o diretório do índice hash extensível
class ExtensibleHash {
private:
    int global_depth;
    std::vector<Bucket*> directory;

    int hash(int key) {
        return key & ((1 << global_depth) - 1);
    }

    void doubleDirectory() {
        int old_size = directory.size();
        global_depth++;
        directory.resize(2 * old_size);
        for (int i = old_size; i < 2 * old_size; i++) {
            directory[i] = directory[i - old_size];
        }
    }

    void splitBucket(int bucket_index) {
        Bucket* old_bucket = directory[bucket_index];
        int old_depth = old_bucket->local_depth;
        if (old_depth == global_depth) {
            doubleDirectory();
        }
        Bucket* new_bucket = new Bucket(old_depth + 1);
        old_bucket->local_depth++;

        std::vector<int> temp_records = old_bucket->records;
        old_bucket->records.clear();

        for (int num : temp_records) {
            int new_index = hash(num);
            directory[new_index]->records.push_back(num);
        }
    }

public:
    ExtensibleHash(int depth) : global_depth(depth) {
        int initial_buckets = 1 << depth;
        for (int i = 0; i < initial_buckets; i++) {
            directory.push_back(new Bucket(depth));
        }
    }

    void insert(int key) {
        int bucket_index = hash(key);
        Bucket* bucket = directory[bucket_index];
        if (bucket->isFull()) {
            splitBucket(bucket_index);
            insert(key);  // Reinsert the key
        }
        else {
            bucket->records.push_back(key);
        }
    }

    void remove(int key) {
        int bucket_index = hash(key);
        auto& records = directory[bucket_index]->records;
        for (auto it = records.begin(); it != records.end(); it++) {
            if (*it == key) {
                records.erase(it);
                return;
            }
        }
    }

    void search(int key) {
        int bucket_index = hash(key);
        Bucket* bucket = directory[bucket_index];
        for (int num : bucket->records) {
            if (num == key) {
                std::cout << "Key " << key << " found in bucket " << bucket_index << "\n";
                return;
            }
        }
        std::cout << "Key " << key << " not found\n";
    }
};

int main() {
    ExtensibleHash exhash(2);  // Inicializa com profundidade global 2

    // Demonstração de inserção, busca e remoção
    exhash.insert(10);
    exhash.insert(20);
    exhash.insert(30);
    exhash.insert(40);
    exhash.search(10);
    exhash.remove(10);
    exhash.search(10);

    return 0;
}
