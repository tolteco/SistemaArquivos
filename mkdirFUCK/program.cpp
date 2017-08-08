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

using namespace std;
using namespace NFS;

void list_source(nfs_sys* sys);
void import(nfs_sys* sys);
void _export(nfs_sys* sys);
void change_directory(nfs_sys* sys);
void make_directory(nfs_sys* sys);
nfs_sys initial_parameters(char *argv[]);
vector<string> split_string(const string& source, const string& delim);

int main (int argc, char *argv[])
{
    if (argc != 3 && argc != 6) {
        cout << "Wrong argument number" << endl << "(device_path -opt)" << endl
             << "opt: f -> format\ti -> initialize" << endl
             << "f block_size group_size n_blocks_per_group" << endl;
        return 1;
    }

    auto sys = initial_parameters(argv);
    string opt;

init:
        cout << "NFS system:" << sys.current_path << "/ ~$ ";
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
            cout << RED_FORE "Comando inválido" RESTORE << endl;
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
    string cmd;
    getline(cin, cmd);

    if (Util::split_string(cmd, ' ').size() != 1)
    {
        return;
    }

    for (auto entry : sys->list_source())
    {
        if (static_cast<unsigned char>(entry.attributes) & static_cast<unsigned char>(entry_flags::DIRECTORY))
        {
            cout << BOLD;
        }
        if (static_cast<unsigned char>(entry.attributes) & static_cast<unsigned char>(entry_flags::EXECUTABLE))
        {
            cout << GREEN_FORE;
        }

        cout.write(entry.name, entry.name_length);
        puts(RESTORE);
    }
}

void make_directory(nfs_sys* sys)
{
    string cmd;
    getline(cin, cmd);

    auto cmd_list = Util::split_string(cmd, ' ');
    if (cmd_list.size() != 2)
    {
        return;
    }

    sys->makedir(cmd_list[1]);
}

void change_directory(nfs_sys* sys)
{
    string cmd;
    getline(cin, cmd);

    auto cmd_list = Util::split_string(cmd, ' ');
    if (cmd_list.size() != 2)
    {
        return;
    }

    sys->changedir(cmd_list[1]);
}

void import(nfs_sys* sys)
{
    string cmd;
    getline(cin, cmd);

    if (Util::split_string(cmd, ' ').size() != 1)
    {
        return;
    }

}

void _export(nfs_sys* sys)
{
    string cmd;
    getline(cin, cmd);

    if (Util::split_string(cmd, ' ').size() != 1)
    {
        return;
    }

}
