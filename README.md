# SistemaArquivos
Terceiro Trabalho de SO - 2017 - NFS - Neo File System

NFS foi desenvolvido na disciplina de SO (Sistemas Operacionais, 2017) na UNIOESTE (Universidade Estadual do Oeste do Paraná), pelos alunos Bruno Luiz Casarotto, Maycon Queiroz de Oliveira e Anderson Bottega.

1. Decisões de projeto

1.1. Organização da repartição

Foi decidido dividir a partição do disco em grupos, e os grupos em blocos, sendo a quantidade de blocos e grupos uma potência de 2.
Com exceção do primeiro grupo, todos os grupos possuem a seguinte estrutura: blocos de dados, blocos reservados para o hashmap e mais blocos de dados. O HASH fica no meio do grupo para diminuir a movimentação do cabeçote.
O primeiro grupo é um caso especial, porque este contém a estrutura INFO no seu primeiro bloco. Portanto ele possui um bloco de dados a menos que os outros grupos.
O diretório root é alocado no bloco seguinte à INFO.

1.2. Organização do HASH

O HASH é um compilado de indexadores de blocos. Há três possíveis sentidos de um valor do HASH:

1. Valor 0 indica que o bloco indexado por aquela posição está livre e pode ser ocupado com dados.

2. Valor diferente do número do bloco indexado indica que os dados da entrada continuam no bloco que o índice aponta.

3. Valor igual ao do número do bloco indexado indica que os dados da entrada terminam neste bloco.

2. Detalhes de implementação

Essa seção é destinada a apresentar os detalhes de implementação do sistema, especificado na linguagem de programação C++.

2.1. INFO
	A estrutura de dados da INFO é descrita, seguindo os seguintes aspectos:

Offset : Tamanho (bytes) : Nome do campo : Descrição
0 : 1 : block_size : Expoente de 2 para obter o tamanho de um bloco.
1 : 1 : n_blocks_per_group : Expoente de 2 para obter o número de blocos que cada grupo comporta
2 : 2 : n_groups : Número de grupos na repartição

2.2. Entry
A estrutura de dados de Entry é descrita como na Tabela 3.

Offset : Tamanho (bytes) : Nome do campo : Descrição
0 : 32 : name : Nome da entrada
32 : 1 : name_length : Tamanho do nome da entrada. Negativo → entrada excluída
33 : 1 : attributes : Atributos da entrada
34 : 6 : creation_time : Data e hora de criação da entrada
40 : 6 : modification_time : Data e hora da última modificação feita na entrada
46 : 6 : last_access_time : Data e hora do último acesso na entrada
52 : 8 : size : Tamanho da entrada em bytes
60 : 4 : first_block : Primeiro bloco do conteúdo da entrada

Os campos attributes, creation_time, modification_time e last_access_time são tipos complexos, sendo attributes uma enum e os campos time uma struct. A enum attributes segue a seguir:

Attributes:
Valor : Nome do atributo : Descrição do atributo
1 : READ_ONLY : Indica que a entrada é somente leitura.
2 : HIDDEN : Indica que a entrada está escondida.
4 : DIRECTORY : Indica que a entrada é um diretório.
8 : FILE : Indica que a entrada é um arquivo.
16 : READ : Indica que é possível ler os dados da entrada.
32 : WRITE : Indica que é possível escrever nos dados da entrada.
64 : EXECUTABLE : Indica que é possível executar a entrada.

Time:
Offset (Bits) : Size (Bits) : Campo : Descrição
0 : 12 : year : Ano (0 - 4095)
12 : 4 : month : Mês
16 : 5 : day : Dia
21 : 5 : hour : Hora
26 : 6 : minute : Minuto
32 : 6 : second : Segundo
38 : 10 : millisecond : Milisegundo

2.3. Diretórios
Os diretórios são organizados da seguinte maneira: a primeira entrada é o ‘.’, diretório corrente, a segunda entrada é o ‘..’, diretório pai, seguido por n entradas de arquivos e diretórios.
Para ler um subdiretório basta ler o(s) bloco(s) de dado(s) da entrada e listar o conteúdo dele.

3. Guia de Programação

Struct para INFO:
typedef struct NFS_info_t
{
  unsigned char block_size;
  unsigned char n_blocks_per_group;
  unsigned char n_groups;
}__attribute__((packed)) info_t;

Struct para os atributos de entrada:
enum class entry_flags : unsigned char
{
  READ_ONLY = 0X01,
  HIDDEN = 0X02,
  DIRECTORY = 0X04,
  FILE = 0X08,
  READ = 0X10,
  WRITE = 0X20,
  EXECUTABLE = 0X40,
}

Struct para date_time:
typedef struct NFS_date_time
{
  unsigned year : 12;
  unsigned month : 4;
  unsigned day : 5;
  unsigned hour : 5;
  unsigned minute : 6;
  unsigned second : 6;
  unsigned millisecond : 10;
}__attribute__((packed)) date_time;

Struct para entrada:
typedef struct NFS_entry_t
{
  char name[32];
  char name_length;
  sfs_entry_attributes attributes;
  sfs_date_time creation_time;
  sfs_date_time modification_time;
  sfs_date_time last_access_time;
  unsigned long long size;
  unsigned int first_block;
}__attribute__((packed)) entry_t;
