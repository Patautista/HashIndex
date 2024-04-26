#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

const std::string IN_PATH = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\in.txt";
const std::string OUT_PATH = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\out.txt";
const std::string HASH_DIR = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\hash_dir";
const std::string CSV_PATH = "C:\\Users\\caleb\\source\\repos\\HashIndex\\HashIndex\\compras.csv";

class Order {
	public:
		int id;
		double value;
		int year;

		Order(double value, int year) : value(value), year(year) {}
		Order(int id, double value, int year) : id(id), value(value), year(year) {}
};

Order findOrderByYear(int target_year) {
	std::ifstream file(CSV_PATH);
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string id_str, value_str, year_str;

		// Supondo que o CSV não tem cabeçalhos e é separado por vírgulas
		std::getline(iss, id_str, ';');
		std::getline(iss, value_str, ';');
		std::getline(iss, year_str, ';');

		int id = std::stoi(id_str);
		double value = std::stod(value_str);
		int year = std::stoi(year_str);

		if (year == target_year) {
			return Order(id, value, year);
		}
	}

	return Order(0,0); // Retorna um optional vazio se nenhum registro for encontrado
}

// Classe que representa um bucket
class Bucket {
public:
	int local_depth;
	std::vector<Order> records;

	Bucket(int depth) : local_depth(depth) {}

	bool isFull() {
		return records.size() >= 3;  // Capacidade máxima do bucket é 3
	}

	bool isEmpty() {
		return records.empty();
	}
	// Método para salvar o bucket em um arquivo
	void save(int index) const {
		// Constrói o nome do arquivo com o índice do bucket
		std::string filename = HASH_DIR + "/" + std::to_string(index) + ".bucket";
		std::ofstream out_file(filename, std::ofstream::trunc);

		if (!out_file.is_open()) {
			std::cerr << "Não foi possível abrir o arquivo para escrita: " << filename << std::endl;
			return;
		}

		// Escreve cada registro no arquivo do bucket
		out_file << std::fixed << std::setprecision(2);

        // Escreve cada registro no arquivo do bucket
        for (const Order& order : records) {
            out_file << order.id << ";" << order.value << ";" << order.year << std::endl;
        }
	}
};

// Classe que representa o diretório do índice hash extensível
class ExtensibleHash {
private:
	int global_depth;
	std::ofstream out_file;
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
			out_file << "DUP_DIR:/" << global_depth << ",<" << global_depth << ">" << std::endl;
		}
		old_bucket->local_depth++;
		Bucket* new_bucket = new Bucket(old_bucket->local_depth);

		int mask = 1 << old_depth;
		for (int i = 0; i < directory.size(); ++i) {
			if ((i & mask) == mask && directory[i] == old_bucket) {
				directory[i] = new_bucket;
			}
		}

		std::vector<Order> temp_records = std::move(old_bucket->records);
		old_bucket->records.clear();

		for (const Order& order : temp_records) {
			if (hash(order.id) == bucket_index) {
				old_bucket->records.push_back(order);
			}
			else {
				new_bucket->records.push_back(order);
			}
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
		out_file.open(OUT_PATH);
		if (!out_file.is_open()) {
			std::cerr << "Não foi possível criar o arquivo de saída out.txt." << std::endl;
			exit(1);
		}
		// Escrever a profundidade global inicial
		out_file << "PG:/" << global_depth << std::endl;

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
			// Buscar no .csv
			auto order = findOrderByYear(key);
			if (order.year != 0) {
				bucket->records.push_back(order);
			}
		}
		directory[bucket_index]->save(bucket_index);

		out_file << "INC:" << key << "/" << global_depth << "," << directory[bucket_index]->local_depth << std::endl;
	}

	void remove(int key) {
		int bucket_index = hash(key);
		auto& records = directory[bucket_index]->records;
		for (auto it = records.begin(); it != records.end(); it++) {
			if ((*it).year == key) {
				records.erase(it);
				directory[bucket_index]->save(bucket_index);
				out_file << "REM:" << key << "/1," << global_depth << "," << directory[bucket_index]->local_depth << std::endl;
				return;
			}
		}
	}

	void search(int key) {
		int bucket_index = hash(key);
		Bucket* bucket = directory[bucket_index];
		for (auto order : bucket->records) {
			if (order.year == key) {
				out_file << "BUS:" << key << "/1" << std::endl;
				return;
			}
		}
		std::cout << "Key " << key << " not found\n";
	}
	void close() {
		out_file.close();
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
	exhash.close();
	return 0;
}
