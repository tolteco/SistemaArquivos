#include <iostream>
#include "NeoFS.h"
#include <cstdlib>
#include <vector>
#include <sstream>

#define BLACK_FORE      "\e[30;1m"
#define RED_FORE        "\e[31;1m"
#define GREEN_FORE      "\e[32;1m"
#define YELLOW_FORE     "\e[33;1m"
#define BLUE_FORE       "\e[34;1m"
#define MAGENTA_FORE    "\e[35;1m"
#define CYAN_FORE       "\e[36;1m"
#define WHITE_FORE      "\e[37;1m"
#define BOLD            "\e[1m"
#define RESTORE         "\e[0m"
#define clear()         cout << "\e[H\e[2J";

using namespace std;
using namespace NFS;

void list_source(nfs_sys* sys);
void import(nfs_sys* sys);
void _export(nfs_sys* sys);
void change_directory(nfs_sys* sys);
void make_directory(nfs_sys* sys);
nfs_sys initial_parameters(char *argv[]);
vector<string> get_parameters();

int main (int argc, char *argv[])
{
    if (argc != 3 && argc != 6) {
        cout << "Wrong argument number" << endl << "(device_path -opt)" << endl
             << "opt: f -> format\ti -> initialize" << endl
             << "f block_size group_size n_blocks_per_group" << endl;
        return 1;
    }

    clear();
    auto sys = initial_parameters(argv);
    string opt;

init:
        cout << "NFS system:" << sys.get_current_path() << "/ ~$ ";
        cin >> opt;

        for (auto i = opt.rbegin(); i != opt.rend(); ++i)
            cin.putback(*i);

        if (opt == "ls")
        {
            // chamar função de list source
            list_source(&sys);
        } else if (opt == "import")
        {
            // chamar função de disco pra pen drive
            import(&sys);
        } else if (opt == "export")
        {
            // chamar função de pen drive pra disco
            _export(&sys);
        } else if (opt == "mkdir")
        {
            // chamar função de criação de diretório
            make_directory(&sys);
        } else if (opt == "cd")
        {
            // chamar função de mudança de diretório
            change_directory(&sys);
        } else if (opt == "exit")
        {
            getline (cin, opt);
            return 0;
        }
        else
        {
            cout << RED_FORE "Invalid argument" RESTORE << endl;
            getline (cin, opt);
        }
    goto init;

    return 0;
}

nfs_sys initial_parameters(char *argv[])
{
    string opt  {argv[2]};
    string path {argv[1]};

    if (opt == "-i")
    {
        return nfs_sys::init_device(path);
    } else if (opt == "-f")
    {
        return nfs_sys::format(path, info_t {
                                            static_cast<unsigned char>(atoi(argv[3])),
                                            static_cast<unsigned char>(atoi(argv[4])),
                                            static_cast<unsigned short>(atoi(argv[5]))});
    }
    else
    {
        cout << "Wrong argument" << endl << "(device_path -opt)" << endl
             << "opt: f -> format\ti -> initialize" << endl
             << "f block_size group_size n_blocks_per_group" << endl;
        exit(EXIT_FAILURE);
    }
}

void list_source(nfs_sys* sys)
{
    if (get_parameters().size() != 1)
    {
        cerr << "Wrong number of arguments" << endl;
        return;
    }

    for (auto entry : sys->list_source())
    {
        if (static_cast<unsigned char>(entry.attributes) & static_cast<unsigned char>(entry_flags::DIRECTORY))
        {
            cout << BLUE_FORE BOLD;
        }
        if (static_cast<unsigned char>(entry.attributes) & static_cast<unsigned char>(entry_flags::EXECUTABLE))
        {
            cout << GREEN_FORE BOLD;
        }

        cout.write(entry.name, entry.name_length);
        puts(RESTORE);
    }
}

void make_directory(nfs_sys* sys)
{
    auto list = get_parameters();

    if (list.size() != 2)
    {
        cerr << "Wrong number of arguments" << endl;
        return;
    }

    sys->makedir(list[1]);
}

void change_directory(nfs_sys* sys)
{
    auto list = get_parameters();
    if (list.size() != 2)
    {
        cerr << "Wrong number of arguments" << endl;
        return;
    }

    sys->changedir(list[1]);
}

void import(nfs_sys* sys)
{
    auto list = get_parameters();
    if (list.size() != 3)
    {
        cerr << "Wrong number of arguments" << endl;
        return;
    }

    sys->disk_to_device(list[1], list[2]);
}

void _export(nfs_sys* sys)
{
    auto list = get_parameters();
    if (list.size() != 3)
    {
        cerr << "Wrong number of arguments" << endl;
        return;
    }

    sys->device_to_disk(list[1], list[2]);
}

vector<string> get_parameters()
{
    vector<string> arr;
    string line;
    auto aspas = 0;
    string arg;

    getline(cin, line);

    for (auto token : Util::split_string(line, ' '))
    {
        if (aspas > 0)
        {
            if (token.front() == '"')
            {
                cerr << "Error quotation mark ballancing" << endl;
                return vector<string>();
            }
            if (token.back() == '"')
            {
                arg += " " + token.substr(0, token.size() - 1);
                arr.push_back(arg);
                arg = "";
                aspas--;
                continue;
            }
            arg += " " + token;
            continue;
        }

        if (token.front() == '"')
        {
            if (token.back() == '"')
            {
                arr.push_back(token.substr(1, token.size() - 2));
                continue;
            }
            aspas++;
            arg += token.substr(1, token.size() - 1);
            continue;
        }

        arr.push_back(token);
    }

    return arr;
}
