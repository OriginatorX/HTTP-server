# include "../include/system.h"
# include <fcntl.h>
# include <stdio.h>
# include <dirent.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>

file_t readFile(const char * path) {  
    file_t file = {
        .state_code = FALSE, 
        .real_quant_bytes = 0
    };
    
    int32_t fd = open(path, O_RDONLY);
    if (fd == ERROR)  {
        fprintf(stderr, "open file: %s failed\n", path);
        return file;
    }

    if (fstat(fd, &file.info) == ERROR) perror("readFile fstat"); 
    
    if (!(file.buffer = (char*) malloc(file.info.st_size)))
        perror("readFile, malloc");
    
    file.real_quant_bytes = read(fd, file.buffer, file.info.st_size);

    if (file.real_quant_bytes == ERROR) {
        fprintf(stderr, "read file %s failed\n", path); return file;
    }
    else if (file.real_quant_bytes == 0)
        printf("%s\n", "file reached");
    if (close(fd) == ERROR) perror("readFile, close");
    
    file.state_code = TRUE;
    return file;
}