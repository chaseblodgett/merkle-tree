#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "hash.h"

#define PATH_MAX 1024

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: ./child_process <blocks_folder> <hashes_folder> <N> <child_id>\n");
        return 1;
    }

    int num_files = atoi(argv[3]);
    int ID = atoi(argv[4]);
    pid_t left_child;
    pid_t right_child;

    if(ID >= num_files-1 && ID <= 2*num_files-2){
        // Leaf Process
        char output_buffer[SHA256_BLOCK_SIZE * 2 + 1];
        char blocks_path[PATH_MAX];
        char hashes_path[PATH_MAX];
        int txt_num = ID - (num_files-1);

        // Get name of blocks file in form path/i.txt and name of hash file in form path/ID.out
        sprintf(blocks_path, "%s/%d.txt", argv[1],txt_num);
        sprintf(hashes_path, "%s/%d.out" , argv[2],ID);
        
        // Open block file to read from and hashes file to write to
        FILE *block_file = fopen(blocks_path, "r");
        FILE *hashes_file = fopen(hashes_path,"w");
        if(block_file == NULL || hashes_file == NULL){
            perror("fopen()");
            exit(EXIT_FAILURE);
        }
        // Get contents of blocks file and hash it to the buffer then write from buffer to hash file
        hash_data_block(output_buffer,blocks_path);
        size_t output_length = strlen(output_buffer);
        size_t output_elements = fwrite(output_buffer,output_length,1,hashes_file);

        if (output_elements < 1) {
            perror("Error writing buffer to hash file");
            fclose(block_file);
            fclose(hashes_file);
            exit(EXIT_FAILURE);
        }

        // Close both files
        fclose(block_file);
        fclose(hashes_file);
        return 0;
    }

    else{
        // Non-leaf process
        char newID[128];
        left_child = fork();

        if(left_child == 0){
            // Left child process
            sprintf(newID, "%d",2*ID+1);
            char *args[] = {"./child_process",argv[1],argv[2],argv[3],newID,NULL};
            if(execv("child_process", args) < 0){
                perror("exec()");
                exit(EXIT_FAILURE);
            }
        }
        else if(left_child < 0){ 
            perror("fork()");
            exit(EXIT_FAILURE);
        }
        
        right_child = fork();
        
        if(right_child == 0){
            // Right child process
            sprintf(newID, "%d", (2*ID) + 2);
            char *args[] = {"./child_process", argv[1], argv[2], argv[3], newID, NULL};
            if(execv("child_process", args) < 0){
                perror("exec()");
                exit(EXIT_FAILURE);
            }
        }
        else if(right_child < 0){
            // Error
            perror("fork()");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for both left and right children
    int status;
    waitpid(left_child,&status,0);
    waitpid(right_child,&status, 0);
    
    // Now all children have finished, starting computing dual hash file data for each non-leaf node

    // Buffers to store hash file data 
    char left_hash[SHA256_BLOCK_SIZE * 2 + 1];
    char right_hash[SHA256_BLOCK_SIZE * 2 + 1];
    char result_hash[SHA256_BLOCK_SIZE * 2 + 1];

    // Paths to left and right child's hash file
    char left_hash_path[PATH_MAX];
    char right_hash_path[PATH_MAX];
    char result_hash_path[PATH_MAX];

    // Compute paths to hash files
    sprintf(left_hash_path,"%s/%d.out",argv[2],(2*ID) + 1);
    sprintf(right_hash_path,"%s/%d.out",argv[2],(2*ID) + 2);
    sprintf(result_hash_path,"%s/%d.out", argv[2],ID);

    // Open left and right files to be read from and create parent hash file to write to
    FILE *left_fd = fopen(left_hash_path,"r");
    FILE *right_fd = fopen(right_hash_path,"r");
    FILE *result_fd = fopen(result_hash_path, "w");

    // Check for any error opening the hash files
    if(left_fd == NULL || right_fd == NULL || result_fd == NULL){
            fclose(right_fd);
            fclose(left_fd);
            fclose(result_fd);
            perror("fopen()");
            exit(EXIT_FAILURE);
    }

    // Get amount of bytes to read from left and right hash
    if(fseek(left_fd,0,SEEK_END) != 0 || fseek(right_fd,0,SEEK_END) != 0){
        perror("fseek()");
        fclose(right_fd);
        fclose(left_fd);
        fclose(result_fd);
        exit(EXIT_FAILURE);
    }

    // Get the left and right child's hash file size and reset files byte offset
    long left_size = ftell(left_fd);
    long right_size = ftell(right_fd);
    if(fseek(left_fd,0,SEEK_SET) != 0 || fseek(right_fd,0,SEEK_SET) != 0 ){
        perror("fseek()");
        fclose(right_fd);
        fclose(left_fd);
        fclose(result_fd);
        exit(EXIT_FAILURE);
    }
   
    // Read data from left and right hashes into buffers
    // Error handle amount of elements read
    size_t elements_read = fread(left_hash,left_size,1,left_fd);
    if (elements_read == 0) {
        if (feof(left_fd)) {
            printf("Reached the end of left_fd");
        } else if (ferror(left_fd)) {
            perror("Error reading from left_fd");
            exit(EXIT_FAILURE);
        }
    }
    elements_read = fread(right_hash,right_size,1,right_fd);
    if (elements_read == 0) {
        if (feof(right_fd)) {
            printf("Reached the end of right_fd");
        } else if (ferror(right_fd)) {
            perror("Error reading from right_fd");
            exit(EXIT_FAILURE);
        }
    }

    // Add null terminator
    right_hash[right_size] = '\0';
    left_hash[left_size] = '\0';

    // Compute hash value from combining children's hashes and write the result to the parent hash file
    compute_dual_hash(result_hash,left_hash,right_hash);
    size_t result_size = sizeof(result_hash);
    size_t elements_written = fwrite(result_hash,result_size,1,result_fd);

    // fwrite error handling
    if (elements_written < 1) {
        perror("Error writing to parent hash file");
        fclose(left_fd);
        fclose(right_fd);
        fclose(result_fd);
        exit(EXIT_FAILURE);
    }

    // Close all open files
    fclose(left_fd);
    fclose(right_fd);
    fclose(result_fd);

    return 0;
}

