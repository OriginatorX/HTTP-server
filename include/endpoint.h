# ifndef ENDPOINT_H
# define ENDPOINT_H

# include <stdint.h>
# include <sys/stat.h>
# include "types.h"
# include "system.h"


Http * init_context(int32_t family, int32_t socket_type, int32_t flag);
Http * http_server(Http *http, char *rootDir, char *port);
bool_t eng_listen(Http *server);
void * get_ip_address(struct sockaddr *sa);
file_t responseFiller(Http *server);
Http * eng_accept(Http *http);
void   leave_context(Http *http);

# endif // ENDPOINT_H
