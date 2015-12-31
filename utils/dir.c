#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <dirent.h>

char *exts = ".jpg.JPG.png.PNG";

int selects(const struct dirent *dir)
{
	int *p;

	char a = strlen(dir->d_name);
	if (a<4) return 0;
	a -= 4;
	p = (int*)&(dir->d_name[a]);

	for (int i=0; i<strlen(exts)/4; i++) {
		if (*p == *((int*)(&exts[i*4]))) return 1;
	}
	return 0;
}

char *getFile(char *dirname, int c)
{
	char *p;
	struct dirent **namelist;

	int r = scandir(dirname, &namelist, selects, alphasort);
	//printf("%s\n%d\n", dirname, r);
	if (r==-1) return 0;

	if (c==-1) c = rand()%r;
	for (int i=0; i<r; i++) {
		//printf("%s\n", namelist[i]->d_name);
		if (i==c) p = strdup(namelist[i]->d_name);
		free(namelist[i]);
	}
	free(namelist);

	return p;
}

int main()
{
	srand((unsigned)time(NULL));
	printf("%s\n", getFile("/usr/share/wallpapers", -1));
}
