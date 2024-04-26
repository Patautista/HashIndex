#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

const std::string IN_PATH = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\in.txt";
const std::string HASH_DIR = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\hash_dir";

class Order {
	public:
		Order(int id, double value) : value(value), year(year) {}
		int id;
		double value;
		int year;
};

// Classe que representa um bucket
class Bucket {
public:
	int local_depth;
	std::string filename;
	std::vector<int> records;

	Bucket(int depth, std::string filename) : local_depth(depth), filename(filename) {}

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
		std::vector<int> temp_records = old_bucket->records;
		Bucket* new_bucket = new Bucket(old_depth + 1, HASH_DIR + "\\" + std::to_string(bucket_index) + ".bucket");
		old_bucket->local_depth++;

		old_bucket->records.clear();

		for (int num : temp_records) {
			int new_index = hash(num);
			directory[new_index]->records.push_back(num);
		}
	}

	void load(const std::string& directory_path) {
		// Itera por todos os arquivos no diretório fornecido
		for (const auto& entry : fs::directory_iterator(directory_path)) {
			if (entry.path().extension() == ".bucket") {
				std::ifstream bucket_file(entry.path());
				std::string line;

				// Lê cada linha do arquivo do bucket (representando um registro)
				while (getline(bucket_file, line)) {
					std::istringstream iss(line);
					int pedido;
					double valor;
					int ano;

					if (iss >> pedido >> valor >> ano) {
						// Insere a chave primária (Pedido #) no hash
						this->insert(pedido);
						// Aqui você pode armazenar os valores e o ano também, dependendo da sua estrutura de bucket
					}
				}
			}
		}
	}

public:
	ExtensibleHash(int depth) : global_depth(depth) {
		int initial_buckets = 1 << depth;
		for (int i = 0; i < initial_buckets; i++) {
			directory.push_back(new Bucket(depth, HASH_DIR + "\\" + std::to_string(i) + ".bucket"));
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

// Função para processar as operações a partir do arquivo de entrada
void processFile(const std::string& filename, ExtensibleHash& exhash) {
	std::ifstream file(filename);
	std::string line;

	// Lê a primeira linha para configurar a profundidade global
	if (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string token;
		if (std::getline(iss, token, '/')) {  // Ignora 'PG'
			if (std::getline(iss, token)) {
				int global_depth = std::stoi(token);
				exhash = ExtensibleHash(global_depth);
			}
		}
	}

	// Processa cada instrução do arquivo
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string command, value_str;
		if (std::getline(iss, command, ':') && std::getline(iss, value_str)) {
			int value = std::stoi(value_str);
			if (command == "INC") {
				exhash.insert(value);
			}
			else if (command == "REM") {
				exhash.remove(value);
			}
			else if (command == "BUS") {
				exhash.search(value);
			}
		}
	}
}

int main() {
	ExtensibleHash exhash(0);  // Profundidade global inicial de 0
	processFile(IN_PATH, exhash);

	return 0;
}
