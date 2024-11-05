#include "../include/utils.h"
#include "math.h"
#include <string.h>

// Create N files and distribute the data from the input file evenly among them
// See section 3.1 of the project writeup for important implementation details
void partition_file_data(char *input_file, int n, char *blocks_folder) {
    
    // Open input file
    FILE *input = fopen(input_file, "r");

    // Check for any errors opening the file
    if(input == NULL){
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    // Check if end of file was actually reached
    if(fseek(input, 0, SEEK_END) != 0){
        printf("fseek()");
        fclose(input);
        exit(EXIT_FAILURE);
    }

    // Get the total size of the file in bytes and reset the file offset to begining of file
    long file_size = ftell(input);
    if(fseek(input,0,SEEK_SET) != 0){
        perror("fseek()");
        fclose(input);
        exit(EXIT_FAILURE);
    }
    if(file_size > 134217728 || file_size < 128 || n > file_size){
        perror("Invalid file size");
        fclose(input);
        exit(EXIT_FAILURE);
    }

    // Extra variables to hold each block size and the path to the new block file
    size_t block_size = floor(file_size / n);

    // Create n-1 new block files of size block_size with data from the input file
    for(int i = 0; i < n - 1; i++){
        // Determine correct path and filename for block
        char filename[PATH_MAX];
        sprintf(filename, "%s/%d.txt", blocks_folder, i);

        // Create new file in the correct path with name i.txt
        FILE *block_file = fopen(filename,"w");
        if(block_file == NULL){
            perror("fopen()");
            fclose(input);
            exit(EXIT_FAILURE);
        }

        // Write contents from input file to block file
        char data[block_size];
        size_t elements_read = fread(data,block_size,1,input);
        if (elements_read == 0) {
            if (feof(input)) {
                printf("Reached the end of input");
            } else if (ferror(input)) {
                perror("Error reading from input");
                exit(EXIT_FAILURE);
            }
        }
        size_t elements_written = fwrite(data,block_size,1,block_file);
        if (elements_written < 1) {
            perror("Error writing to block file");
            fclose(block_file);
            exit(EXIT_FAILURE);
        }

        // Close the block file
        fclose(block_file);
    }

    // Create last block file with leftover bytes from the input file
    char filename[PATH_MAX];
    sprintf(filename, "%s/%d.txt", blocks_folder, n-1);
    FILE *block_file = fopen(filename,"w");
    if(block_file == NULL){
        perror("fopen()");
        fclose(input);
        exit(EXIT_FAILURE);
    }
    block_size = floor(file_size / n) + (file_size % n);

    char data[block_size];
    size_t elements_read = fread(data,block_size,1,input);
    if (elements_read == 0) {
            if (feof(input)) {
                printf("Reached the end of input");
            } else if (ferror(input)) {
                perror("Error reading from input");
                exit(-1);
            }
        }
    size_t elements_written = fwrite(data,block_size,1,block_file);
    if (elements_written < 1) {
            perror("Error writing to block file");
            fclose(block_file);
            fclose(input);
            exit(EXIT_FAILURE);
        }

    // Close files and return
    fclose(block_file);
    fclose(input);
    return;
}


// ##### DO NOT MODIFY THIS FUNCTION #####
void setup_output_directory(char *block_folder, char *hash_folder) {
    // Remove output directory
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"rm", "-rf", "./output/", NULL};
        if (execvp(*argv, argv) < 0) {
            printf("ERROR: exec failed\n");
            exit(1);
        }
        exit(0);
    } else {
        wait(NULL);
    }

    sleep(1);

    // Creating output directory
    if (mkdir("output", 0777) < 0) {
        printf("ERROR: mkdir output failed\n");
        exit(1);
    }

    sleep(1);

    // Creating blocks directory
    if (mkdir(block_folder, 0777) < 0) {
        printf("ERROR: mkdir output/blocks failed\n");
        exit(1);
    }

    // Creating hashes directory
    if (mkdir(hash_folder, 0777) < 0) {
        printf("ERROR: mkdir output/hashes failed\n");
        exit(1);
    }
}
