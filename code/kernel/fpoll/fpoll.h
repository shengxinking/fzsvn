/**
 *
 *
 *
 *
 *
 */

typedef enum fpoll_cmd {
	FPOLL_CTL_ADD,
	FPOLL_CTL_MOD,
	FPOLL_CTL_DEL,
} fpoll_cmd_e

typedef struct fpoll_fd {
	int		fd;		/* client fd */
	int		family;		/* socket family: AF_INET/AF_INET6 */
	struct sockaddr addr;		/* client address */
	u_int16_t	port;		/* client port */
	int		errno;		/* errno if failed */
} fpoll_fd_t;

typedef struct fpoll_data {
	int		fd;
	void		*data;
	size_t		max;
	size_t		len;
	int		errno;
} fpoll_data_t;

typedef struct fpoll_event {
	int		fd;
	int		events;
	fpoll_cmd_e	cmd;
	int		errno;
} fpoll_event_t;

extern int 
fpoll_accept(int fd, fpoll_accept_t *afd, int nafd, int timeout);


extern int 
fpoll_recv(fpoll_data_t *data, int ndata, int timeout);


extern int 
fpoll_send(fpoll_data_t *data, int ndata, int timeout);


extern int 
fpoll_ctlv(int fd, fpoll_event_t *event, int nevent);

extern int 
fpoll_create(int max);

extern int 
fpoll_close(int fd);



