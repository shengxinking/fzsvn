/**
 *
 *
 *
 *
 *
 */

typedef struct fpoll_accept {
	int		fd;		/* client fd */
	int		family;		/* socket family: AF_INET/AF_INET6 */
	struct sockaddr addr;		/* client address */
	u_int16_t	port;		/* client port */
} fpoll_accept_t;

typedef struct fpoll_packet {
int		fd;
size_t		max;
size_t		len;
} fpoll_packet_t;


typedef struct fpoll_data {
	int		fd;
char
} fpoll_data_t;


extern int 
fpoll_accept(int fd, acceptfd_t *afd, int nafd, int timeout);


extern int 
fpoll_recv(fpoll_data_t *data, int ndata, int timeout);


extern int 
fpoll_send(fpoll_data_t *data, int ndata, int timeout);




