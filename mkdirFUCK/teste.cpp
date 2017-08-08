#include <iostream>
#include <string>
#include <cstdlib>
#include "NeoFS.h"
#include <vector>

using namespace std;
/*
struct st {
	int a;
	char b;
};

class cl {
public:
	int a;
	char b;
};

int main2()
{
	struct st a{4, 'c'};
	cl b{4, 'c'};
	
	FILE *fp;
	fp = fopen("a.bin", "w");
	
	cout << sizeof(a) << endl << sizeof(b) << endl;
	fwrite(&a, sizeof(struct st), 1, fp);
	fclose(fp);
	
	fp = fopen("b.bin", "w");
	fwrite(&b, sizeof(cl), 1, fp);
	fclose(fp);
	return 0;
}
*/

void format_cmd(vector<string>& cmd)
{

}

int main()
{
//    auto sys = NFS::nfs_sys::format("/dev/sdb1", NFS::info_t{9, 15, 61});
    auto sys = NFS::nfs_sys::init_device("/dev/sdb1");

    for (auto entry : sys.list_source())
    {
        cout.write(entry.name, entry.name_length);
        cout << endl;
    }

    sys.disk_to_device("/home/administrador/program.c");


    for (auto entry : sys.list_source())
    {
        cout.write(entry.name, entry.name_length);
        cout << endl;
    }

//    sys.disk_to_device();
//    sys.device_to_disk("teste.txt", "../resultado.txt");

	return 0;
}
