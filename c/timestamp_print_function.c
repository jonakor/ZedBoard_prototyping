#include <stdio.h>

void timestamps_print(u32 **arrayPointer);

int main() {
  u32 **timestampArrayPtr;
  u32 arraySize = 2300;
  u32 unixStamp = 1583240308;

  timestampArrayPtr = (u32 **)calloc(TIMESTAMP_ARRAY_SIZE, sizeof(u32 *));

  for (int i = 0; i < TIMESTAMP_ARRAY_SIZE; i++) {
    timestampArrayPtr[i] = (u32 *)calloc(2, sizeof(u32));
  }

  timestamps_print(timestampArrayPtr, arraySize, unixStamp);
}

void timestamps_print(u32 **arrayPointer, u32 TIMESTAMP_ARRAY_SIZE, u32 UNIX_TIMESTAMP) {
    
    FILE * fp;
    //fp = fopen("path/to/file/%s.txt",(string)(UNIX_TIMESTAMP));
    fp = fopen ("c:\\temp\\1.txt","w");
    printf("%u", UNIX_TIMESTAMP)

    for (int i = 0; i < TIMESTAMP_ARRAY_SIZE; i++) {
        printf("%9u\t%9u\r\n", arrayPointer[i][0], arrayPointer[i][1])
    }

    fclose(fp);
    return 0;
}