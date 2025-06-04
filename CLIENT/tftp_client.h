#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char server_ip[INET_ADDRSTRLEN];
} tftp_client_t;

typedef enum
{
    NORMAL = 1,
    OCTET = 2,
    NET_ASCII = 3
}MODE;

extern MODE mode;

typedef enum
{
    SUCCESS,FAILURE
} status;

// // Function prototypes
void connect_to_server(tftp_client_t *client, char *ip, int port); //1
void put_file(tftp_client_t *client, tftp_packet *packet);
void get_file(tftp_client_t *client, tftp_packet *packet);
void disconnect(tftp_client_t *client);
void process_command(tftp_client_t *client, char *command);

void send_file(tftp_client_t *client, tftp_packet *packet);
void receive_file(tftp_client_t *client, tftp_packet *packet);


status ipv4_validation(char ip[]);
status validate_file(char filename[]);
void select_mode();

#endif