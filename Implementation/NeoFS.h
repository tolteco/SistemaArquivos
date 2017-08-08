#ifndef _NEOFS_H
#define _NEOFS_H
#include <iostream>
#include <stdio.h>
#include <vector>

//Simplistic file system's namespace
//Aqui você provavelmente vai achar coisas úteis
//para mecher com o sistema de arquivos
namespace NFS
{
	//Atributos que uma entrada do sistema pode ter
	//como politica de l/e e o tipo da entrada
	//(1 byte)
	enum class entry_flags : unsigned char
	{
		//Diz que o conteúdo da entrada só pode ser lido
		//e não escrito e/ou executado
		READ_ONLY = 0x01,

		//Diz que a entrada é escondida e não será
		//mostrada a menos que seja explicitamente
		//indicado a apresentação de entradas
		//escondidas
		HIDDEN = 0x02,

		//Diz que a entrada é um diretório
		DIRECTORY = 0x04,

		//Diz que a entrada é um tipo de
		//arquivo
		FILE = 0x08,

		//Diz que é ou não possivel de ler
		//o conteúdo da entrada
		READ = 0x10,

		//Diz se é ou não possivel de escrever
		//no conteúdo da entrada
		WRITE = 0x20,

		//Diz se a entrada é possivel de ser
		//executada
		EXECUTABLE = 0x40/*,
						 XXXXXXXXXXXX= 0X80*/
	};

#pragma pack(push, 1)
	//Estrutura do cabeçalho do sistema de arquivos
	//aqui (possivelmente) possui as informações
	//necessárias para fazer o uso do SFS
	//(4 bytes)
    typedef struct NFS_info_t
	{
		//2 ^ block_size = verdadeiro tamanho do bloco
		unsigned char	block_size;

		//2 ^ n_blocks_per_group = numero de blocos em cada grupo
		unsigned char	n_blocks_per_group;

		//numero de grupos na partição
		unsigned short	n_groups;
    }info_t;

	//Representa data e hora desde 01/01/0000 am 00:00:00.000
	//até 31/12/4095 pm 11:59:59.999
	//(6 bytes)
    typedef struct NFS_date_time
	{
		//0 - 4095
		unsigned year : 12;

		//1 - 12 (não faz sentido ter outro valor)
		unsigned month : 4;

		//1 - 31 (não faz sentido ter outro valor)
		unsigned day : 5;

		//0 - 11 (não faz sentido ter outro valor)
		unsigned hour : 5;

		//0 - 59 (não faz sentido ter outro valor)
		unsigned minute : 6;

		//0 - 59 (não faz sentido ter outro valor)
		unsigned second : 6;

		//0 - 999(não faz sentido ter outro valor)
		unsigned millisecond : 10;
    }date_time_t;

	//Entrada do sistema, contém informações úteis (risos)
	//para lidar com o SFS
	//(64 bytes)
    typedef struct NFS_entry_t
	{
		//nome da entrada
		char				name[32];

		//tamanho do nome da entrada
		//-name_length => entrada excluído			
		//zero => entrada livre
		char				name_length;

		//atributos da entrada
		entry_flags			attributes;

		//data e hora da criação da entrada
		date_time_t			creation_time;

		//data e hora da ultima modificação da entrada
		date_time_t			modification_time;

		//data e hora do ultimo acesso da entrada
		date_time_t			last_access_time;

		//tamanho do arquivo em bytes
		unsigned long long	size;

		//número do primeiro bloco do arquivo
		unsigned int		first_block;
    }entry_t;

#pragma pack(pop)

	class nfs_sys
	{
		/*
         * tirar comentário do privado
         *
		 * Todo List
		 * 1 - create directory
		 * 2 - list directory
		 * 3 - navigate through directories
		 * 4 - rename entry
		 * 5 - delete entry
		 * 6 - move entry
		 * 7 - copy entry
		 *
		 * consertar lista de blocos da hash tá bugado, sério
         *
         * ok
         * write_hash(); testado
         * read_hash() testado
         * delete_hash(); testado
         * expand(); testado
         * free_blocks_for() testado
         * format() testado
         * init_device() testado
         * disk_to_device();
         *
         * terminar
         * makedir();
		 */
	public:
		// construtor para um dispositivo já existente
		static nfs_sys init_device(const std::string& device_path);

		// formatodor para um novo dispositivo
        static nfs_sys format(const std::string& device_path, const info_t& info);

		// destrutor da classe, desaloca o device
		~nfs_sys();

		// cria um subdiretório dentro do atual
		void makedir(std::string name);
		
		// lista as entradas de uma pasta
		std::vector<entry_t> list_source() const;
		
		// escreve do disco para o device
		void disk_to_device(const std::string& source) const;
		
		// escreve do device para o disco
		// terminar de implementar
		void device_to_disk(const std::string& file, const std::string& path) const;

//	private:
		// tamanho do bloco em bytes
		const unsigned int block_size;

		// numero de blocos por grupo
		const unsigned int n_blocks_pg;

		// numero de grupos do dispositivo
		const unsigned int n_groups;

		// tamanho do grupo em bytes
		const unsigned long long group_size;

		// quantidade de blocos da hash de um grupo
		const unsigned int hash_span;

		// numero de blocos deslocados do inicio
		// do grupo para chegar na HASH
		const unsigned int init_hash;

		// tamanho do dispositivo
		const unsigned long deviceSize;

		// numero de entradas em um bloco da HASH
		const unsigned int max_hash_entries;

		// bloco de início do diretorio atual
		const unsigned int current_directory;

		// numero maximo de entradas em um bloco
		const unsigned int max_entries;
		
		// localização do primeiro bloco da root
		const unsigned int root_block;

		// dispositivo NFS
		FILE* device;

		// caminho atual
		std::string current_path;

		// contrutor para os dois factories estaticos
		nfs_sys(FILE* device, const info_t& info);

        // retorna uma lista de n blocos livres na HASH
		std::vector<unsigned int> free_blocks_for(unsigned int quantity) const;

		// retorna uma lista de blocos da entrada
		std::vector<unsigned int> read_hash(unsigned int first) const;

		// escreve uma lista de blocos na hash
		void write_hash(unsigned int *blocks, const int& quantity) const;

		// escreve uma lista de blocos na hash
		void write_hash(std::vector<unsigned int> blocks) const;

		// remove as entradas da hash
		void delete_hash(unsigned int first) const;

		// aumenta a entrada atual para ocupar os blocos
        void expand(unsigned int last, std::vector<unsigned int> blocks) const;
        
        // escreve a entrada no diretorio
        void append_entry(unsigned int first, const entry_t& entry) const;

		// retorna o endereço de inicio do bloco
		inline unsigned long long block_addr(unsigned int block) const;

		// retorna o endereço do bloco na tabela HASH
		inline unsigned long long block_hash_addr(unsigned int block) const;
	};

	date_time_t current_time();

	char* format_time(date_time_t time);

	inline entry_flags operator | (entry_flags left, entry_flags right);
}

#endif
