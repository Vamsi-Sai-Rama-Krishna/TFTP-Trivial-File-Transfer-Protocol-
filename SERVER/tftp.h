/* Common file for server & client*/

#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>

#define PORT 6969
#define BUFFER_SIZE 516  // TFTP data packet size (512 bytes data + 4 bytes header)
#define TIMEOUT_SEC 5    // Timeout in seconds

// TFTP OpCodes
typedef enum {
    RRQ = 1,  // Read Request  client recive and sever sends get case
    WRQ = 2,  // Write Request  client sends and sever revives put case
    DATA = 3, // Data Packet
    ACK = 4,  // Acknowledgment
    ERROR = 5 // Error Packet
} tftp_opcode;


typedef enum
{
    NORMAL = 1,     // 1 time - 512 bytes
    OCTET = 2,      // 1 time  - 1 byte
    NET_ASCII = 3   // 1 time - 1 line inserted with \r before \n
}MODE;

extern MODE mode;
// TFTP Packet Structure
typedef struct {
    uint16_t opcode; // Operation code (RRQ/WRQ/DATA/ACK/ERROR)
    union {
        struct {
            char filename[256];
            int mode;  // Typically "octet"
        } request;  // RRQ and WRQ
        struct {
            uint16_t block_number;
            char data[512];
        } data_packet; // DATA
        struct {
            uint16_t block_number;
        } ack_packet; // ACK
        // struct {
            //     uint16_t error_code;
            //     char error_msg[512];
            // } error_packet; // ERROR
        } body;
    } tftp_packet;

    typedef enum
    {
        SUCCESS,FAILURE
    } status;


    void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet );
    void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet );
    
    status validate_file(char filename[]);
    
    #endif // TFTP_H 