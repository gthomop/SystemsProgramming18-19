/****** dirControl.c *******/
#include "dirControl.h"

int remove_dir(char *dir_path){

  DIR *dir = opendir(dir_path);
  if (dir == NULL && errno == ENOENT){
    return 2;
  }
  else if (dir == NULL){
    perror("Remove_dir - Opendir()");
    return 1;
  }

  struct dirent *dirp = NULL;
  //the absolute path of a file to be deleted
  char *path_buf = NULL;
  //the length of the directory path this function was called to delete
  size_t path_len = strlen(dir_path);

  while( (dirp = readdir(dir)) != NULL){
    if ( !(strcmp(dirp->d_name, ".")) || !(strcmp(dirp->d_name, ".."))){
      continue;     //no need to delete or explore them
    }

    path_buf = malloc(sizeof(char) * (path_len + strlen(dirp->d_name) + 1 + 1));
    strcpy(path_buf, dir_path);
    strcat(path_buf, "/");
    strcat(path_buf, dirp->d_name);

    //if file is a directory
    if ( (is_dir(path_buf)) ){
      if ( (remove_dir(path_buf)) ){    //if the directory deletion was not successful
        fprintf(stderr, "Directory %s inside %s could not be deleted. Aborting deletion of directory.\n", dirp->d_name, dir_path);
        free(path_buf);
        closedir(dir);
        return 1;
      }
      free(path_buf);
      path_buf = NULL;
    }
    else{
      if ( (unlink(path_buf)) ){
        perror("remove_dir - unlink(path_buf)");
        free(path_buf);
        closedir(dir);
        return 1;
      }
      free(path_buf);
      path_buf = NULL;
    }
  }

  closedir(dir);
  rmdir(dir_path);

  return 0;
}

int is_dir(char *path){
  struct stat stat_buf = {0};

  if ( (stat(path, &stat_buf)) ){
    perror("is_dir - stat()");
    return 2;
  }

  if ( (stat_buf.st_mode & S_IFMT) == S_IFDIR){
    return 1;
  }
  else return 0;
}

char *get_fifo_name(char *common_dir_path, char *id1, char *id2){
  char *fifo_name = malloc(sizeof(char) * ( strlen(common_dir_path) + strlen("/") + strlen(id1) +\
    strlen("_to_") + strlen(id2) + strlen(".fifo") + 1) );

  strcpy(fifo_name, common_dir_path);
  strcat(fifo_name, "/");
  strcat(fifo_name, id1);
  strcat(fifo_name, "_to_");
  strcat(fifo_name, id2);
  strcat(fifo_name, ".fifo");

  return fifo_name;
}

char *append_to_rel_path(char *dir, char *file){
  char *path_buf = malloc(sizeof(char) * ( strlen(dir) + strlen(file) + 2) );
  strcpy(path_buf, dir);
  strcat(path_buf, "/");
  strcat(path_buf, file);

  return path_buf;
}