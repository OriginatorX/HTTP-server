# ifndef TYPES_H
# define TYPES_H

# include <sys/socket.h>
# include <netdb.h>
# include <sys/stat.h>

# define PAGE            4096
# define ERROR           (-1)
# define SOCKET_ERROR    (-1)
# define MAX_CONNECTIONS  128
# define CRLF      "\r\n\r\n"

typedef int32_t socket_t;

typedef enum {
    FALSE = 0,
    TRUE  = 1
} bool_t;

typedef struct mime_t {
    char *type;
} mime_t;

typedef struct file_t {
    char        *buffer;
    struct stat info;
    int32_t     real_quant_bytes;
    bool_t      state_code;
} file_t;

typedef struct http_request {
    char    *buffer;
    int32_t buf_len;
    int32_t read_bytes;
} http_request;

typedef struct http_response {
    char    *buffer;
    int32_t buf_len;
    int32_t write_bytes;
} http_response;

typedef struct multiplex_t {
    int32_t  ready_fd;
    int32_t  max_len;
    socket_t socket_fd;
    socket_t connection_fd;
    int32_t  max_fd;
    int32_t  client[FD_SETSIZE];
    fd_set   readSet;
    fd_set   allSet; 
} multiplex_t;

typedef struct Http {
    bool_t          state_code;
    socket_t        listener;
    multiplex_t     *multiplexor;
    struct addrinfo hints;
    struct addrinfo *service;

    socklen_t remote_user_addr_len;
    struct sockaddr_storage remote_user;
    
    char  remote_user_addr_buf[INET6_ADDRSTRLEN];
    char  staticDir[32]; 
    char* port;

    http_request  req;
    http_response res;
} Http;


#endif // TYPES_H