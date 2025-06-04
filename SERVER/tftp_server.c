/*
    Name: T.Vamsi Krishna
    ID: 24024C_179
    Description: To implement an TFTP[Trivial File Tranfer Protocol] for LAN based Networks that helps users/clients
                 to share files among server with operations like:
                 1.connect
                 2.Put
                 3.Get
                 4.MOde
                 
    Date : 28/05/2024

    ** Server Part **
    server works/does operations like read or write to files based on client opted MODE
*/


#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<errno.h>

int fd;
MODE mode=NORMAL;
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() 
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    // socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;


    printf("Server is waiting. .. . ..\n");
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Set socket timeout option
    //TODO Use setsockopt() to set timeout option
    
    
    // Set up server address
    server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(5001); 
    
    
    // Bind the socket
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)); 

    
    socklen_t client_len = sizeof(struct sockaddr_in);
    
    
    // Main loop to handle incoming requests
    while (1) 
    {
        printf("TFTP Server listening on port ...\n");
        //recive request
        int n = recvfrom(sockfd, &packet, sizeof(tftp_packet), 0, (struct sockaddr *)&client_addr, &client_len);
        
        if(n == -1)
        {
            perror("recvfrom");
            return -1;
        }
        //extract the information[file name and opcode] in the request recieved and start operation based on the info
        handle_client(sockfd, client_addr, client_len, &packet);
    }
    
    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    // Extract the TFTP operation (read or write) from the received packet
    // and call send_file or receive_file accordingly
    
    //if request packet opcade is WRQ  => do write operation i.e receive from client
    if(packet->opcode == WRQ)
    {
        //also update mode[global variable] as the packet is structures nested inside union the data will be lost for next operations
        mode = packet->body.request.mode;
        
        //validate file is present or not
        //file not present create =>
        if(validate_file(packet->body.request.filename) == FAILURE)
        {
            //if valdidate failed means file is not present so close it and create a file with same name i.e received from client
            close(fd);  //close the fd
            //if file present => clear the contetnt
            fd = open(packet->body.request.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
        }
        else
        {
            //close the fd
            close(fd);

            //re-open in write mode
            fd = open(packet->body.request.filename, O_WRONLY | O_TRUNC, 0644);
        }
        
        //update acknowledgement packet
        if(fd == -1)
        {
            //transmit the acknowledgment packet updated with FAILURE
            packet->body.ack_packet.block_number = FAILURE;
            sendto(sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);  
        }
        else
        {
            //transmit acknowledgment packet updated with SUCCESS
            packet->body.ack_packet.block_number = SUCCESS;
            sendto(sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);   
        }
        
        //receive the file 
        printf("Receiving File .. .. ..\n");
        receive_file(sockfd, client_addr, client_len, packet);
        printf("File Received...!\n");
    }
    //if request packet opcode is RRQ => do read operation i.e send file to client 
    else if(packet->opcode == RRQ)
    {
        //also update mode here
        mode = packet->body.request.mode;
        
        if(validate_file(packet->body.request.filename) == FAILURE)
        {
            //send acknowledgement to client that file opening or file not present
            packet->body.ack_packet.block_number = FAILURE;
            sendto(sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);
            return;
        }
        else
        {
            // send acknowledgement as SUCCESS to client that file is present and opened successfully
            packet->body.ack_packet.block_number = SUCCESS;
            sendto(sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);
        }

        //send the file
        printf("Sending File .. .. ..\n");
        send_file(sockfd, client_addr, client_len, packet);
        printf("File Sent...!\n");
    }
    else
    {
        printf("**  Invalid OPCODE requested to Server  **\n");
        return;
    }
}