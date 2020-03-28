#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int timestamps_to_file(char *, unsigned int);

int main(void)
{
	char *path = malloc(64 * sizeof(char));
	unsigned int frames = 0;

	printf("Enter file name: ");
	scanf("%s", path);
	printf("\nEnter numer of frames: ");
	scanf("%u", &frames);
	printf("\n");

	printf("%i bytes didn't make it\n", timestamps_to_file(path, frames));

	free(path);
	return 0;
}

/*	Function to write timestamps to file
	Arguments:
		char *path , Filepath including name
		unsigned int n_frames , number of frames to file
	Return:
		Error code or 0
*/
int timestamps_to_file(char *path, unsigned int n_frames) 
{
	size_t bytes = n_frames*2*sizeof(unsigned int);
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
	pointer = malloc(bytes);
	if (pointer == NULL){
		printf("Failed allocating memory.\n");
		return -ENOMEM;
	}
	unread = read(fdArr, pointer, bytes);
	if (unread > 0)
	{
		printf("Couldn't read %zu of %u bytes\n", unread, bytes);
	}

	// Print to file
	fdFile = fopen(path, "w");
	if (fdFile == NULL){
		printf("Failed creating new file %s.\n", path);
		return -ENOENT;
	}
	for (int i = 0; i < n_frames; ++i) {
		fprintf(fdFile, "%9u,%9u\n", pointer[i*2], pointer[i*2 + 1]);
	}
	
	fclose(fdFile);
	free(pointer);
	close(fdArr);

	return unread;
}
