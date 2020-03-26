#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <unistd.h>

#define N_FRAME_MAX 2300
#define TIMESTAMP_ARRAY_SIZE (N_FRAME_MAX * 2 * 4) // Two timestamps per frame, four bytes per timestamp.

int timestamps_to_file(char *);

int main(void){
	char *ts_unix = malloc(22 * sizeof(char));
	char *path = malloc(50 * sizeof(char));
	printf("Enter file name:");
	scanf("%s", ts_unix);
	printf("\n");
	sprintf(path, "/home/root/%s.txt", ts_unix);

	timestamps_to_file(path);
	free(path);

	return 0;
}

int timestamps_to_file(char *path) {
	int fdArr;
	unsigned int *pointer;
	ssize_t unread;
	FILE *fdFile;
	

	// Open timestamp device
	fdArr = open("/dev/timestamps", O_RDONLY | O_SYNC);
	if (fdArr < 0){
		printf("Failed opening timestamps.\n");
		return fdArr;
	}
	pointer = calloc(N_FRAME_MAX*2, sizeof(unsigned int));
	if (pointer == NULL){
		printf("Failed allocating memory.\n");
		return -ENOMEM;
	}
	unread = read(fdArr, pointer, TIMESTAMP_ARRAY_SIZE);
	if (unread > 0)
	{
		printf("Couldn't read %zu of %u bytes\n", unread, TIMESTAMP_ARRAY_SIZE);
	}

	// Print to file
	fdFile = fopen(path, "w");
	if (pointer == NULL){
		printf("Failed creating new file %s.\n", path);
		return -ENOENT;
	}
	for (int i = 0; i < 100; ++i)  {
		fprintf(fdFile, "%9u\t%9u\n", pointer[i*2], pointer[i*2 + 1]);
	}
	
	fclose(fdFile);
	free(pointer);
	close(fdArr);
}
