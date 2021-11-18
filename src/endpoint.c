# include <stdio.h>
# include <stdlib.h>
# include <errno.h>
# include <string.h>
# include <fcntl.h>

# include <arpa/inet.h>
# include <unistd.h>
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <dirent.h>

# include "../include/endpoint.h"

#define NUMBER_OF_MIME_TYPE 10

extern char not_404_found[512];

Http * init_context(int32_t family, int32_t socket_type, int32_t flag) {

    Http *http = (Http*) malloc(sizeof(Http));
    if (!http) { 
        perror("malloc");  
        return http; 
    }
    http->state_code  = FALSE;
    
    http->req.buffer  = (char*) malloc(PAGE / 4); 
    http->req.buf_len = PAGE / 4;
    
    http->res.buffer  = (char*) malloc(PAGE * 4); 
    http->res.buf_len = PAGE * 4;
   
    if (!http->req.buffer) 
        perror("init_context, req->buffer malloc");
    if (!http->res.buffer) 
        perror("init_context, res->buffer malloc");

    memset(&http->hints, 0, sizeof http->hints);
    http->hints.ai_family   = AF_UNSPEC;  
    http->hints.ai_socktype = SOCK_STREAM;
    http->hints.ai_flags    = AI_PASSIVE;
    http->state_code        = TRUE;  
    return http;
}

Http* http_server(Http *http, char *rootDir, char *port) {

    errno                     = 0;
    int32_t status            = 0;
    http->state_code          = FALSE;
    http->port                = port;
    struct addrinfo *iterator = NULL;

    for (int8_t j = 0, *i = rootDir; *i != '\0'; i++, j++) 
        http->staticDir[j] = rootDir[j];

    if ((http->multiplexor = (multiplex_t*) malloc(sizeof(multiplex_t))) == NULL) 
        perror("http_server, storage malloc");
    
    memset(http->multiplexor->client, -1, sizeof http->multiplexor->client);

    if ((status = getaddrinfo (NULL, port, &http -> hints, &http -> service)) == ERROR) {
        fprintf(stderr, "Get address information failed %s\n", gai_strerror(status)); 
        return http;
    }

    for (iterator = http->service; iterator != NULL; iterator = iterator->ai_next) {
        if ((http->listener = socket(iterator->ai_family, iterator->ai_socktype, 
            iterator->ai_protocol)) == ERROR) {
                perror("http_server, socket"); continue;
        }
        if (setsockopt(http->listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == ERROR) {
            perror("http_server, setsockopt"); 
            return http;
        }
        if (bind(http->listener, iterator->ai_addr, iterator->ai_addrlen) == ERROR) {
            perror("http_server, bind"); 
            return http;
        }   
        break;
    }   
    freeaddrinfo(http->service);

    if (iterator == NULL) { 
        perror("http_server not address information"); 
        return http; 
    } 

    FD_ZERO(&http->multiplexor->allSet);
    FD_SET(http->listener, &http->multiplexor->allSet);

    http->state_code = TRUE;
    return http;
}

bool_t eng_listen(Http *http) {
    if (listen(http->listener, MAX_CONNECTIONS) == ERROR) {
        perror("http_server, listen"); 
        return FALSE;
    }
    printf("server run on port %s \n", http->port);

    size_t i         = 0;
    size_t increment = 1;
    http->state_code = FALSE;
    http->multiplexor->max_fd = http->listener;

    while (TRUE) {
        http->multiplexor->readSet = http->multiplexor->allSet;
        
        if ((http->multiplexor->ready_fd = select(http->multiplexor->max_fd + 1, 
            &http->multiplexor->readSet, NULL, NULL, NULL)) == ERROR) {
                perror("handle_connection, select"); exit(EXIT_FAILURE);
        }
        if (FD_ISSET(http->listener, &http->multiplexor->readSet)) {
            if ((http->multiplexor->connection_fd = accept(http->listener, 
                (struct sockaddr*) &http->remote_user, &http->remote_user_addr_len)) == -1) {
                    perror("accept"); continue;
            }
            
            printf("New request %s on port %d\n№ %ld\n", 
                inet_ntop(http->remote_user.ss_family, 
                    get_ip_address((struct sockaddr*) &http->remote_user),
                    http->remote_user_addr_buf, INET6_ADDRSTRLEN), 
                http->multiplexor->connection_fd, increment++
            );

            for (i = 0; i < FD_SETSIZE; i++) {
                if (http->multiplexor->client[i] < 0) { 
                    http->multiplexor->client[i] = http->multiplexor->connection_fd; 
                    break;
                }
            }
            FD_SET(http->multiplexor->connection_fd, &http->multiplexor->allSet);

            if (http->multiplexor->connection_fd > http->multiplexor->max_fd) 
                http->multiplexor->max_fd = http->multiplexor->connection_fd;
            if (i > http->multiplexor->max_len) 
                http->multiplexor->max_len = i;
            if (http->multiplexor->ready_fd <= 0) 
                continue;
        }
        
        for (size_t i = 0; i <= http->multiplexor->max_len + 1; i++) {
            if ((http->multiplexor->socket_fd = http->multiplexor->client[i]) < 0) 
                continue;

            if (FD_ISSET(http->multiplexor->socket_fd, &http->multiplexor->readSet)) {
                if ((http->req.read_bytes = read(http->multiplexor->socket_fd, 
                    http->req.buffer, http->req.buf_len)) == -1) 
                {
                    perror("received"); 
                    close(http->multiplexor->socket_fd);
                    FD_CLR(http->multiplexor->socket_fd, &http->multiplexor->allSet);
                    http->multiplexor->client[i] = -1;
                }
                else if (http->req.read_bytes == 0) {
                    printf("%s\n", "data none");
                    close(http->multiplexor->socket_fd);
                    FD_CLR(http->multiplexor->socket_fd, &http->multiplexor->allSet); 
                    http->multiplexor->client[i] = -1;
                }
                else {
                    printf("%s\n", http->req.buffer);

                    responseFiller(http);
                    
                    if ((http->res.write_bytes = write(http->multiplexor->socket_fd, 
                        http->res.buffer, http->res.buf_len)) == -1) 
                    {
                        close(http->multiplexor->socket_fd);
                        FD_CLR(http->multiplexor->socket_fd, &http->multiplexor->allSet);
                        http->multiplexor->client[i] = -1;
                        perror("send"); 
                    }
                }
                if (http->multiplexor->ready_fd <= 0) break;
            } 
        }   
    } 
    close(http->listener);
    return TRUE;
}

void * get_ip_address(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);
    else 
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

mime_t mime_type(char *d_name) {

    size_t i = 0;
    size_t j = 0;
    size_t k = 0;

    mime_t mime;
    char file_extension[8];

    memset(file_extension, '\0', sizeof file_extension);

    for (k = 0; d_name[k] == '/'; k++);
    if (d_name[k] != '\0') {
        for (k = 0; d_name[k] != '.'; k++);
        for (i = k; d_name[i] != '\0'; i++, j++) 
            file_extension[j]  = d_name[i];
    } 
    else  
        file_extension[0]  = '/';

    char* keys_val[NUMBER_OF_MIME_TYPE][2] = {
        {"/"    ,    "text/html"        },
        {".html",    "text/html"        },
        {".htm" ,    "text/html"        },
        {".css" ,    "text/css"         },
        {".js"  ,    "text/javascript"  },
        {".jpg" ,    "image/jpeg"       },
        {".png" ,    "image/png"        },
        {".ico" ,    "image/x-icon"     },
        {".mp4" ,    "video/mp4"        },
        {".json",    "application/json" }
    };

    for (size_t i = 0; i < NUMBER_OF_MIME_TYPE; i++) {
        if (strcmp(keys_val[i][0], file_extension) == 0) {
            mime.type = keys_val[i][1];
            break;
        }
    }
    
    return mime;
}

void parser(Http *server, char* req_path, char* catalog) {
 
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;

    for (int8_t i = 0, *j = server->staticDir; *j != '\0'; i++, j++) 
        catalog[i] = server->staticDir[i];

    for (i = 0; server->req.buffer[i] != '/'; i++);
    for (j = i; server->req.buffer[j] != ' '; j++, k++)
        req_path[k] = server->req.buffer[j];    
}

file_t responseFiller(Http *http) {
    file_t file;
    char file_extension[16];
    char* code = "200 OK";

    char req_path[64];
    char catalog[32];

    memset(req_path, '\0', sizeof req_path);
    memset(catalog, '\0', sizeof catalog);

    parser(http, req_path, catalog);

    strcat(catalog, req_path);
    if (strcmp("/", req_path) == 0) 
        strcat(catalog, "index.html");

    mime_t mime = mime_type(req_path);
    file = readFile(catalog);

    if (file.state_code == FALSE) {
        if ((file.buffer = (char*) malloc(sizeof not_404_found)) == NULL) 
            perror("parser, malloc");

        for (size_t i = 0; i < sizeof not_404_found; i++)
            file.buffer[i] = not_404_found[i];

        printf("%ld\n", sizeof not_404_found);

        file.real_quant_bytes = sizeof not_404_found; 
        mime.type = "text/html";
        code = "404\tNOT\tFOUND";
    }

    int http_len = snprintf (
        http->res.buffer,
        http->res.buf_len,
        "HTTP/1.1 %s OK\n"
        "Content-Type: %s\n"
        "Content-Length: %d" CRLF,
        code,
        mime.type,
        file.real_quant_bytes 
    );

    if (file.real_quant_bytes > http->res.buf_len) {
        if ((http->res.buffer = (char*) realloc(http->res.buffer, file.real_quant_bytes + http_len)) == NULL)
            perror("parser, realloc");
    }
    http->res.buf_len = file.real_quant_bytes + http_len;

    memcpy(http->res.buffer + http_len, file.buffer, file.real_quant_bytes);
    free(file.buffer);
    
    file.buffer = 0x0;
    return file;
}

void leave_context(Http *http) {
    if (http->req.buffer)  
        free(http->req.buffer);
    if (http->res.buffer)  
        free(http->res.buffer);
    if (http->multiplexor) 
        free(http->multiplexor);
    if (http)              
        free(http);
}