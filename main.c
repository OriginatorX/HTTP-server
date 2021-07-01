# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>

# include "include/endpoint.h"

int main(int argc, char** const argv) {
    
    Http * http = init_context(AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);

    if (argc != 2) {
        printf("%s [port-number]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    http_server(http, "wwwroot", argv[1]);
    eng_listen(http);

    leave_context(http);
    return EXIT_SUCCESS;
}
