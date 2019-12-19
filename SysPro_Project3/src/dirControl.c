/****** dirControl.c *******/
#include "dirControl.h"

/* Makes a list of the files of this client. Uses two pointers of pointers to list nodes in order to 
 lessen the complexity by not searching serially the list to find its end every time a node is going to be
 appended */
int get_file_list(char *dir_path, file_list **list, file_list **last, int *num_of_files, short first_rec) {
    //first_rec is needed in case the directory of the client is empty
    DIR *dir = opendir(dir_path);

    if (dir == NULL){
        char err_str[] = "get_file_list:The directory could not be opened\n";
        write(2, err_str, strlen(err_str));

        return 1;
    }

    struct dirent *dirp = NULL;
    //the path of a file
    char *path_buf = NULL;
    short file_found = 0;

    while( (dirp = readdir(dir)) != NULL){
        if ( !(strcmp(dirp->d_name, ".")) || !(strcmp(dirp->d_name, "..")) || !(strcmp(dirp->d_name, "__RSRVD"))){
            continue;
        }

        if (first_rec == 0) {
            path_buf = malloc(sizeof(char) * (strlen(dir_path) + strlen(dirp->d_name) + 1 + 1));
            strcpy(path_buf, dir_path);
            strcat(path_buf, "/");
            strcat(path_buf, dirp->d_name);
        }
        else {
            path_buf = malloc(sizeof(char) * (strlen(dirp->d_name) + 1));
            strcpy(path_buf, dirp->d_name);
        }
        

        //if file is a directory
        if ( (is_dir(path_buf)) == 1 ){
            file_found = 1;
            if ( (get_file_list(path_buf, list, last, num_of_files, 0)) ){
                char str1[] = "get_file_list:Directory ";
                char str2[] = " inside ";
                char str3[] = " could not be sent.\n";
                size_t len = strlen(str1) + strlen(dirp->d_name) + strlen(str2) + strlen(dir_path) + strlen(str3) + 1;
                char *err_str = malloc(sizeof(char) * len);
                strcpy(err_str, str1);
                strcat(err_str, dirp->d_name);
                strcat(err_str, str2);
                strcat(err_str, dir_path);
                strcat(err_str, str3);
                write(2, err_str, len);
                free(path_buf);
                closedir(dir);
                return 1;
            }
            free(path_buf);
            path_buf = NULL;
        }
        else{
            file_found = 1;
            (*num_of_files)++;

            if ((*list) == NULL) {            
                (*list) = malloc(sizeof(file_list));
                (*last) = (*list);
                (*last)->previous = NULL;
            }
            else {
                (*last)->next = malloc(sizeof(file_list));
                (*last)->next->previous = (*last);
                (*last) = (*last)->next;
            }
            (*last)->next = NULL;
            (*last)->this_tuple = malloc(sizeof(file_tuple));
            memset((*last)->this_tuple, 0, sizeof(file_tuple));
            strcpy((*last)->this_tuple->pathname, path_buf);
            int fd = open(path_buf, O_RDONLY);
            char *version = find_version(fd);
            close(fd);
            strcpy((*last)->this_tuple->version, version);

            free(version);
            free(path_buf);
            path_buf = NULL;
        }
    }

    if (file_found == 0 && first_rec == 0) {      //the directory was empty
        (*num_of_files)++;
        if ((*list) == NULL) {
            (*list) = malloc(sizeof(file_list));
            (*last) = (*list);
            (*last)->previous = NULL;
        }
        else {
            (*last)->next = malloc(sizeof(file_list));
            (*last)->next->previous = (*last);
            (*last) = (*last)->next;
        }
        (*last)->next = NULL;
        (*last)->this_tuple = malloc(sizeof(file_tuple));
        strcpy((*last)->this_tuple->pathname, dir_path);
        strcpy((*last)->this_tuple->version, "DIR");
    }

    closedir(dir);

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

/* Found on stackoverflow.com */
char *find_version(int fd) {
    unsigned char c[MD5_DIGEST_LENGTH];
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    char *ret = malloc(sizeof(char) * (32 + 1) );       //the hash is 128 bytes long so in hex we need 32 characters to display it

    MD5_Init(&mdContext);
    while ( (bytes = read(fd, data, 1024)) != 0 ) {
        MD5_Update(&mdContext, data, bytes);
    }
    MD5_Final(c, &mdContext);

    char buffer[3];
    sprintf(buffer, "%02x", c[0]);
    strcpy(ret, buffer);
    for (int i = 1; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(buffer, "%02x", c[0]);
        strcat(ret, buffer);
    }

    return ret;
}

char *get_version(char *fileName) {
    int file_fd = open(fileName, O_RDONLY);

    if (file_fd != -1) {        //if the file exists
        return find_version(file_fd);
    }

    return NULL;
}