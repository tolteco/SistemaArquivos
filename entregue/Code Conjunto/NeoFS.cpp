#include "NeoFS.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sstream>

using namespace NFS;
using namespace std;

// rever os calculos de inicializaçao
// contrutor para os dois factories estaticos
nfs_sys::nfs_sys(FILE *device, const info_t &info)
    : block_size(static_cast<unsigned int>(pow(2, info.block_size))),
      n_blocks_pg(static_cast<unsigned int>(pow(2, info.n_blocks_per_group))),
      n_groups(info.n_groups),
      group_size(static_cast<unsigned long long>(pow(2, info.block_size + info.n_blocks_per_group))),
      // numero de blocos / (tamanho de um bloco / tamanho de um inteiro)
      // da erro se o blocks_per_group for menor do que o block size em log2( tamanho do int)
      hash_span(pow(2, info.n_blocks_per_group - info.block_size + static_cast<int>(log2(sizeof(unsigned int))))),
      // inicio do hash é o bloco do meio do grupo menos a metade do tamanho da HASH
      init_hash(static_cast<unsigned int>(pow(2, info.n_blocks_per_group - 1) - this->hash_span / 2)),
      deviceSize(this->group_size * info.n_groups),
      max_hash_entries(this->block_size / sizeof(unsigned int)),
      current_directory(1),
      max_entries(this->block_size / sizeof(entry_t)),
      root_block(1),
      device(device),
      current_path(vector<string>())
{
}

// destrutor da classe, desaloca o device
nfs_sys::~nfs_sys()
{
    // fecha o arquivo do device
    fclose(this->device);
}

// cria um subdiretório dentro do atual
// terminar de implementar
void nfs_sys::makedir(const std::string& path)
{
    // 1 pra caso o diretório atual estaja cheio
    // outro para o diretório que será criado
    auto free_blk = free_blocks_for(1);

    // não tem entrada e não tem bloco disponível
    if (!free_blk.size())
        return;

    auto time = current_time();
    auto file_name = get_file_name(path);
    auto file_block = get_last_dir_block(path.substr(0, path.size() - file_name.size()));

    entry_t entry { "",
                    static_cast<unsigned char>(file_name.size()),
                entry_flags::DIRECTORY |
                entry_flags::READ |
                entry_flags::WRITE,
                time,
                time,
                time,
                0, free_blk[0]
                  };
    memcpy(entry.name, file_name.c_str(), file_name.length());

    append_entry(file_block, entry);

    fseek(this->device, block_addr(free_blk[0]), SEEK_SET);

    // informação básica de um diretório
    entry_t basic[] = {
        entry_t{ ".", 1,
                 entry_flags::HIDDEN |
                 entry_flags::DIRECTORY |
                 entry_flags::READ |
                 entry_flags::WRITE,
                 time,
                 time,
                 time,
                 0, free_blk[0] },

        entry_t{ "..", 2,
                 entry_flags::HIDDEN |
                 entry_flags::DIRECTORY |
                 entry_flags::READ |
                 entry_flags::WRITE,
                 time,
                 time,
                 time,
                 0, file_block }
    };

    fwrite(&basic, sizeof(entry_t), 2, this->device);

    write_hash(free_blk);
}

// formatodor para um novo dispositivo
// versão final mudar de w+ para r+ o modo de abertura do arquivo
nfs_sys nfs_sys::format(const std::string &device_path, const info_t &info)
{
    auto fp = fopen(device_path.c_str(), "w+");

    auto sys = nfs_sys(fp, info);

    // bota o ponteiro do arquivo no inicio dele
    fseek(sys.device, 0, SEEK_SET);

    // aloca um bloco zerado para limpar o dispositivo
    auto empty_block = static_cast<char*>(calloc(sys.block_size, sizeof(char)));

    // percorre os grupos e os blocos dentro de cada grupo zerando os blocos
    for (auto i = 0u; i < sys.n_groups; i++)
    {
        for (auto j = 0u; j < sys.n_blocks_pg; j++)
        {
            fwrite(empty_block, sizeof(char), sys.block_size, sys.device);
        }
    }

    // escreve a INFO do dispositivo
    fseek(sys.device, 0, SEEK_SET);
    fwrite(&info, sizeof(info_t), 1, sys.device);

    // move para o primeiro bloco e pega o tempo atual
    fseek(sys.device, sys.block_addr(sys.root_block), SEEK_SET);
    auto current_t = current_time();

    // informação básica de um diretório
    entry_t basic[] = {
        entry_t{ ".", 1,
                 entry_flags::HIDDEN |
                 entry_flags::DIRECTORY |
                 entry_flags::READ |
                 entry_flags::WRITE,
                 current_t,
                 current_t,
                 current_t,
                 0, sys.root_block },

        entry_t{ "..", 2,
                 entry_flags::HIDDEN |
                 entry_flags::DIRECTORY |
                 entry_flags::READ |
                 entry_flags::WRITE,
                 current_t,
                 current_t,
                 current_t,
                 0, sys.root_block }
    };

    fwrite(&basic, sizeof(entry_t), 2, sys.device);

    sys.write_hash(vector<unsigned int>{sys.root_block});

    return sys;
}

// construtor para um dispositivo já existente
nfs_sys nfs_sys::init_device(const std::string& device_path)
{
    auto device = fopen(device_path.c_str(), "r+");

    info_t info;
    fread(&info, sizeof(info_t), 1, device);

    return nfs_sys(device, info);
}

// retorna uma lista de n blocos livres na HASH
vector<unsigned int> nfs_sys::free_blocks_for(unsigned int quantity) const
{
    vector<unsigned int> list;
    auto actualGroup = 0u;
    auto hash = static_cast<unsigned int*>(malloc(this->block_size));

    fseek(this->device, this->init_hash * this->block_size, SEEK_SET);

    // itera pelos grupos enquanto não tiver sido alocado o espaço requirido
    while (quantity && actualGroup < this->n_groups)
    {
        // itera pelos blocos da hash enquanto não tiver sido alocado e espaço requirido
        for (auto i = 0u; i < this->hash_span && quantity; i++)
        {
            fread(hash, sizeof(unsigned int), this->max_hash_entries, this->device);

            // itera pela hash
            for (auto index = 0u; index < this->max_hash_entries && quantity; index++)
            {
                if (!hash[index])
                {
                    --quantity;
                    // bloco mapeado na posição atual é obtido pela expressao
                    // quantidade de blocos do inicio do bloco da hash mais 1 mais
                    // quantidade de blocos já percorridos desde o inicio da hash vezes
                    // numero de entradas por bloco mais grupo atual vezes tamanho da hash
                    // vezes numero de entrado por bloco
                    list.push_back(index + 1 + (i * this->max_hash_entries) + (actualGroup * this->max_hash_entries * this->hash_span));
                }
            }
        }
        fseek(this->device, this->group_size - (this->hash_span * this->block_size), SEEK_CUR);
    }

    free(hash);

    // se quantity for diferente de zero não tem espaço suficiente para n blocos
    if (quantity != 0)
        return vector<unsigned int>();

    return list;
}

// retorna uma lista de blocos da entrada
vector<unsigned int> nfs_sys::read_hash(unsigned int first) const
{
    vector<unsigned int> arr;
    auto crr_blk = first; // current block
    // ReSharper disable once CppInitializedValueIsAlwaysRewritten
    auto last_blk = 0u;

    do {
        arr.push_back(crr_blk);

        fseek(this->device, block_hash_addr(crr_blk), SEEK_SET);

        last_blk = crr_blk;
    } while (fread(&crr_blk, sizeof(unsigned int), 1, this->device) && last_blk != crr_blk && crr_blk);

    // se crr_blk for 0 o arquivo está corrompido
    if (crr_blk == 0)
        return vector<unsigned int>();

    return arr;
}

// escreve uma lista de blocos na hash
void nfs_sys::write_hash(unsigned int * blocks, const int & quantity) const
{
    write_hash(vector<unsigned int>{blocks, blocks + quantity});
}

// escreve uma lista de blocos na hash
void nfs_sys::write_hash(std::vector<unsigned int> blocks) const
{
    auto index = 1u;
    const auto quantity = blocks.size();

    fseek(this->device, block_hash_addr(blocks[0]), SEEK_SET);

    while (index < quantity)
    {
        fwrite(&blocks[index], sizeof(unsigned int), 1, this->device);
        fseek(this->device, block_hash_addr(blocks[index]), SEEK_SET);
        ++index;
    }

    fwrite(&blocks.back(), sizeof(unsigned int), 1, this->device);
}

// aumenta a entrada atual para ocupar os blocos
void nfs_sys::expand(unsigned int last, std::vector<unsigned int> blocks) const
{
    if (!blocks.size())
        return;

    fseek(this->device, block_hash_addr(last), SEEK_SET);

    for (auto block : blocks)
    {
        fwrite(&block, sizeof(unsigned int), 1, this->device);
        fseek(this->device, block_hash_addr(block), SEEK_SET);
    }

    fwrite(&blocks.back(), sizeof(unsigned int), 1, this->device);
}

vector<entry_t> nfs_sys::list_source() const
{
    vector<entry_t> arr;
    entry_t entry;

    for (auto block : read_hash(this->current_directory))
    {
        fseek(this->device, block_addr(block), SEEK_SET);

        for (auto i = 0u; i < this->max_entries; i++)
        {
            fread(&entry, sizeof(entry_t), 1, this->device);
            if (entry.name_length <= 0)
                continue;
            arr.push_back(entry);
        }
    }

    return arr;
}

void nfs_sys::device_to_disk(const string& source, const string& dest) const
{
    auto dest_file = fopen(dest.c_str(), "wb");

    if (dest_file == nullptr)
    {
        cerr << "Invalid destiny" << endl;
        return;
    }

    auto entry = get_entry(source);

    if (entry.first_block == 0)
    {
        cerr << "Invalid source" << endl;
        return;
    }

    auto buff = static_cast<char*>(malloc(this->block_size));
    auto size = entry.size;

    for (auto block : read_hash(entry.first_block))
    {
        fseek(this->device, block_addr(block), SEEK_SET);
        fread(buff, sizeof(char), size >= this->block_size ? this->block_size : size, this->device);
        fwrite(buff, sizeof(char), size >= this->block_size ? this->block_size : size, dest_file);
        size -= this->block_size;
    }

    fclose(dest_file);
    free(buff);
}

void nfs_sys::disk_to_device(const string& source, const string& dest)
{
    auto src = fopen(source.c_str(), "rb");

    if (src == nullptr)
    {
        cerr << "Invalid source" << endl;
        return;
    }

    auto file_name = get_file_name(dest);
    auto path_block = get_last_dir_block(dest.substr(0, dest.length() - file_name.length()));

    if (path_block == 0)
    {
        cerr << "Invalid destiny" << endl;
        return;
    }

    fseek(src, 0, SEEK_END);

    auto size = ftell(src);

    fseek(src, 0, SEEK_SET);

    auto blocks_size = size / this->block_size + (size % this->block_size ? 1 : 0);
    auto buff = static_cast<char*>(malloc(this->block_size));
    auto blocks = free_blocks_for(blocks_size);
    auto current_t = current_time();

    if (blocks_size != blocks.size())
    {
        cerr << "There is no free space for such a file" << endl;
        return;
    }

    auto entry = entry_t { "", static_cast<char>(file_name.length()),
            entry_flags::FILE |
            entry_flags::READ |
            entry_flags::WRITE,
            current_t,
            current_t,
            current_t,
            size, blocks[0]
    };

    memcpy(entry.name, file_name.c_str(), file_name.length());

    append_entry(path_block, entry);

    for (auto block : blocks)
    {
        fseek(this->device, block_addr(block), SEEK_SET);

        auto read_size = fread(buff, sizeof(char), this->block_size, src);

        fwrite(buff, sizeof(char), read_size, this->device);
    }

    write_hash(blocks);

    fclose(src);
    free(buff);
}

// escreve a entrada no diretorio
void nfs_sys::append_entry(unsigned int first, const entry_t& entry) const
{
    auto buff = static_cast<entry_t*>(malloc(this->block_size));
    auto blocks = read_hash(first);

    for (auto block : blocks)
    {
        fseek(this->device, block_addr(block), SEEK_SET);
        fread(buff, sizeof(entry_t), this->max_entries, this->device);

        for (auto i = 0u; i < this->max_entries; i++)
        {
            if (buff[i].name_length < 1)
            {
                // volta max_entries - i na tabela para escrever
                fseek(this->device, static_cast<int>((i - this->max_entries) * sizeof(entry_t)), SEEK_CUR);
                fwrite(&entry, sizeof(entry_t), 1, this->device);
                free(buff);
                return;
            }
        }
    }

    auto free_one = free_blocks_for(1);
    expand(blocks.back(), free_one);

    fseek(this->device, block_addr(free_one[0]), SEEK_SET);
    fwrite(&entry, sizeof(entry_t), 1, this->device);
    free(buff);
}

// return only the file name, without the path
string nfs_sys::get_file_name(const string& fullname) const
{
    auto index = fullname.find_last_of("/");

    return fullname.substr(index + 1);
}

unsigned int nfs_sys::get_last_dir_block(const std::string& path, bool changedir)
{
    if (path == "/")
    {
        if (changedir)
        {
            this->current_directory = this->root_block;
            append_path("");
        }
        return this->root_block;
    }

    auto last_path = this->current_path;
    auto last_dir = this->current_directory;

    auto block = this->current_directory;
    auto buff = static_cast<entry_t*>(malloc(this->block_size));
    bool found_block;

    if (path[0] == '/')
    {
        block = this->root_block;
        if(changedir)
            append_path("");
    }

    auto folders = Util::split_string(path, '/');

    for (auto folder : folders)
    {
        found_block = false;

        if (folder == "")
            continue;
        for (auto index : read_hash(block))
        {
            fseek(this->device, block_addr(index), SEEK_SET);
            fread(buff, sizeof(entry_t), this->max_entries, this->device);

            for (auto i = 0u; i < this->max_entries; i++)
            {
                if (string{ buff[i].name, buff[i].name_length } == folder)
                {
                    block = buff[i].first_block;
                    found_block = true;
                    if(changedir){
                        this->current_directory = block;
                        append_path(folder);
                    }

                    break;
                }
            }
            if (found_block)
                break;
        }
        if (!found_block)
        {
            if (changedir){
                this->current_directory = last_dir;
                this->current_path = last_path;

            }
            free(buff);
            return 0;
        }
    }
    free(buff);
    return block;
}

const entry_t nfs_sys::get_entry(const string &path) const
{
    if (path == "/")
    {
        cerr << "Invalid path" << endl;
        return entry_t();
    }

    auto block = this->current_directory;
    auto buff = static_cast<entry_t*>(malloc(this->block_size));
    bool found_block;
    entry_t entry;

    if (path[0] == '/')
        block = this->root_block;

    auto folders = Util::split_string(path, '/');

    for (auto folder : folders)
    {
        found_block = false;

        if (folder == "")
            continue;
        for (auto index : read_hash(block))
        {
            fseek(this->device, block_addr(index), SEEK_SET);
            fread(buff, sizeof(entry_t), this->max_entries, this->device);

            for (auto i = 0u; i < this->max_entries; i++)
            {
                if (string{ buff[i].name, buff[i].name_length } == folder)
                {
                    entry = buff[i];
                    block = buff[i].first_block;
                    found_block = true;

                    break;
                }
            }
            if (found_block)
                break;
        }
        if (!found_block)
        {
            cerr << "Invalid path" << endl;
            free(buff);
            return entry_t();
        }
    }
    free(buff);
    return entry;
}

void nfs_sys::changedir(const string &path)
{
    get_last_dir_block(path, true);
}

void nfs_sys::append_path(const string& path)
{
    if (path == "")
    {
        current_path.clear();
        return;
    }

    if (path == "..")
    {
        if (current_path.size() > 0)
            current_path.pop_back();
        return;
    }

    if (path == ".")
        return;

    current_path.push_back(path);
}

const string nfs_sys::get_current_path() const
{
    static string path;

    path = "";

    for (auto s : this->current_path)
    {
        path += "/" + s;
    }

    return path;
}

// retorna o endereço do bloco na tabela HASH
inline unsigned long long nfs_sys::block_hash_addr(unsigned int block) const
{
    --block;
    return this->init_hash * this->block_size + (block % this->n_blocks_pg) * sizeof(unsigned int) + this->group_size * (block / this->n_blocks_pg);
}

// retorna o endereço de inicio do bloco
inline unsigned long long nfs_sys::block_addr(unsigned int block) const
{
    return block * this->block_size;
}

inline entry_flags NFS::operator | (entry_flags left, entry_flags right)
{
    return static_cast<entry_flags>(static_cast<unsigned char>(left) |
                                    static_cast<unsigned char>(right));
}

inline entry_flags NFS::operator & (entry_flags left, entry_flags right)
{
    return static_cast<entry_flags>(static_cast<unsigned char>(left) &
                                    static_cast<unsigned char>(right));
}

date_time_t NFS::current_time()
{
    long            ms; // Milliseconds
    struct timespec spec;
    struct tm *time_desc;

    time_desc = localtime(&spec.tv_sec);
    ms = spec.tv_nsec / 1000000; // Convert nanoseconds to milliseconds

    date_time_t curr;

    curr.millisecond = ms;
    curr.second = time_desc->tm_sec;
    curr.minute = time_desc->tm_min;
    curr.hour = time_desc->tm_hour;
    curr.day = time_desc->tm_mday;
    curr.month = time_desc->tm_mon + 1;		// struct tm tem valores de 0 - 11
    curr.year = time_desc->tm_year + 1900;	// struct tm começa a contar em 1900

    return curr;
}

char* NFS::format_time(date_time_t time)
{
    auto str = static_cast<char*>(malloc(24 * sizeof(char)));
    sprintf(str, "%02u/%02u/%04u %02u:%02u:%02u.%03u",
            time.day, time.month, time.year, time.hour, time.minute,
            time.second, time.millisecond);
    return str;
}

vector<string> Util::split_string(const string &source, const char &delim)
{
    vector<string> arr;
    stringstream buffer {source};
    string item;

    while (getline(buffer, item, delim))
    {
        arr.push_back(item);
    }

    return arr;
}
