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
    auto sys = NFS::nfs_sys::init_device("teste.bin");
	
    sys.disk_to_device("teste.txt");

	return 0;
}
