#include "utils.h"
#include "print_tree.h"
#include <string.h>

// ##### DO NOT MODIFY THESE VARIABLES #####
char *blocks_folder = "output/blocks";
char *hashes_folder = "output/hashes";
char *visualization_file = "output/visualization.txt";


int main(int argc, char* argv[]) {
    if (argc != 3) {
        // N is the number of data blocks to split <file> into (should be a power of 2)
        // N will also be the number of leaf nodes in the merkle tree
        printf("Usage: ./merkle <file> <N>\n");
        return 1;
    }

    // TODO: Read in the command line arguments and validate them
    char *input_file = argv[1];
    int n = atoi(argv[2]);

    if(input_file == NULL || n <= 0){
        perror("Error: Invalid Command Line Arguments");
        exit(-1);
    }

    setup_output_directory(blocks_folder, hashes_folder);
    partition_file_data(input_file, n, blocks_folder);

    pid_t child_pid = fork();

    if(child_pid == 0){
        // This is the root node for the merkle tree 
        // Generate arguments to pass and call exec(), beginning the merkle tree creation process
        char *args[] = {"./child_process", blocks_folder, hashes_folder, argv[2], "0" , NULL};
        execv("child_process", args);
        // If here then error when calling exec()
        perror("exec()");
        exit(-1);
    }
    else if(child_pid < 0){
        perror("fork()");
        exit(-1);
    }

    // Since this is the root child node, this wait call waits for the entire merkle tree to finish
    int status;
    waitpid(child_pid,&status,0);

    // ##### DO NOT REMOVE #####
    #ifndef TEST_INTERMEDIATE
        // Visually display the merkle tree using the output in the hashes_folder
        print_merkle_tree(visualization_file, hashes_folder, n);
    #endif

    return 0;
}