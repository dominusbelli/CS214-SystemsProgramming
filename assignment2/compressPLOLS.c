#include <signal.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "lols.h"
//include everything just in case

//signal handler here??? need??
void handler(int sig) {
  volatile

}

void compressR_LOLS(char* file, int parts) {
  //**--- Routine file checking
  FILE *fp;
  // file is the name/path to the file
  // Ensures the file is valid
  if (access(file, F_OK) == -1) {
    printf("%s\n", "File does not exist");
    return 1;
  }
  fp = fopen(file, "r");

  // ensures that file streams can open the given file
  if (fp == NULL) {
    printf("%s\n", "File could not be opened.");
    return 1;
  }
  //**--- End routine file checking
  printf("%s\n", "Finished routine checks");

  FileData *data = (FileData *)malloc(sizeof(FileData));
  printf("%s\n", "apparently thats legal");
  data->name = getFileName(file);
  data->path = getOutputFile(file);
  data->fullpath = file;

  /*parallel processing starts here*/
  /*not sure how connect lols.c soooo
   *i'm gonna assume i have a lot of vars that i don't
   *like inputLength and numberOfParts*/

  pid_t * children = (pid_t *) malloc(sizeof(pid_t)*numberOfParts);

  //parent process waiting on all children
  pid_t p;
  int status;
  int j = 0;
  //need to check that children is populated
  while(j < numberOfParts) {
    p = waitpid(children[j], &status, WNOHANG);
    j++;
  }


  //dis where forking starts
  pid_t pid;

  int i = 0;
  while (i < numberOfParts) {
    pid = fork();

    if (pid > 0) {
      children[i] = pid;
      i++;
      //sleep(1); ?
    } else if (pid < 0){
      //error handling do later
      printf("Fork has failed.\n");
      //perror("Fork has failed");
      exit(1);
    } else {
      //child process, compress here

      write();
    }



  }

  free(children);
}



int main(int argc, char const *argv[]) {
  FILE *fp;
  FILE *count;

  // argv[2] is the number of parts
  // Ensures we get a number of parts
  if (argc != 3) {
    printf("%s\n", "Error: Incorrect amount of arguments. Please use:\n\"./asst2 [text file to be compressed] [Number of parts]\"");
    return 1;
  }
  int numberOfParts = atoi(argv[2]);
  printf("%ld\n", numberOfParts);

  // argv[1] is the name/path to the file
  // Ensures the file is valid
  if (access(argv[1], F_OK) == -1) {
    printf("%s\n", "File does not exist");
    return 1;
  }
  fp = fopen(argv[1], "r");
  count = fopen(argv[1], "r");

  // ensures that both file streams can open the given file
  if (fp == NULL || count == NULL) {
    printf("%s\n", "File could not be opened.");
    return 1;
  }

  // Count how big the input is
  int inputLength;
  while (fgetc(count) != EOF) {
    inputLength++;
  }

  fclose(fp);
  fclose(count);

  // Compression starts here
  compressR_LOLS(argv[1], argv[2]);
}
