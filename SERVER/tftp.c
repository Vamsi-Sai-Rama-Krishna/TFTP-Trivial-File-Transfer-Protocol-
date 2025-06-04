/* Common file for server & client */

#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<errno.h>

extern int fd;
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    // Implement file sending logic here
    // Implement file sending logic here

    packet->body.data_packet.block_number = 1;  //block number intillailly 1

    int read_ret = 1;
    int Net_ascii_byte_count = 0; 
    int read_ret_for_net_ascii = 1;
    int net_Ascii_flag = 0;
    char ch;

    while( read_ret != 0 )
    {
        if(mode == NORMAL)   //fixed packet size of 512 bytes
        {
            read_ret = read(fd, packet->body.data_packet.data ,512);
        }
        else if(mode == OCTET)      //read 1 character and transmit it
        {
            read_ret = read(fd, packet->body.data_packet.data,1);
        }
        else if(mode == NET_ASCII)      //handles \n and \n and 512 bytes
        {
            while((read_ret_for_net_ascii = read(fd, &ch ,1)) > 0)
            {
                //now check character is \n or not 
                if(ch == '\n' && Net_ascii_byte_count <= 510 )
                {
                    packet->body.data_packet.data[Net_ascii_byte_count++] = '\r';       //insert \r 
                    packet->body.data_packet.data[Net_ascii_byte_count++] = '\n';       // and then \n for linux systems
                }
                else if(ch != '\n' && Net_ascii_byte_count < 512)
                {
                    packet->body.data_packet.data[Net_ascii_byte_count++] = ch; //increment the byte count aswell as update the character
                }
                else
                {
                    break; // break to send the data and to handle edge case
                }

                if(Net_ascii_byte_count == 512)
                {
                    break;      //when byte count reaches 512 terminate the loop and send that packet of 512 bytes data
                }
            }
            //when file offset reached EOF in NET-ASCII handling last chunk of data and EOF
            if(read_ret_for_net_ascii <= 0 && Net_ascii_byte_count != 0)  
            {
                //send the data less then 512  and file offset reached EOF
                packet->body.data_packet.block_number = Net_ascii_byte_count; 
                sendto(sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);  //send that last chunk of data
                
                recvfrom(sockfd,(void *)packet,sizeof(tftp_packet),0,(struct sockaddr *) &client_addr, &client_len);
                net_Ascii_flag = 1; //flag will be set when read ret becomes zero , indicating EOF so transmits 
                Net_ascii_byte_count = 0; //update count value to data packet block number as file offset reached EOF 
                
                read_ret = 0;   //update read ret to break the outer loop
            }
            //handle for empty file in NET-ASCII mode
            else if(read_ret_for_net_ascii == 0 && Net_ascii_byte_count == 0)
            {
                read_ret = 0;
            }
        }
        else
        {
            printf("Invalid Mode\n");
            return;
        }
        
        //send the data filled in the packet 
        if (mode == NET_ASCII && (net_Ascii_flag == 1 || Net_ascii_byte_count == 512))
        {
            packet->body.data_packet.block_number = Net_ascii_byte_count; //update count value to data packet block number as file offset reached EOF 
            sendto(sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);
            
            //reset after sending the data
            net_Ascii_flag = 0;
            Net_ascii_byte_count = 0;
        }
        else
        {
            packet->body.data_packet.block_number = read_ret;
            sendto(sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);
        }

        //receive acknowledgment
        recvfrom(sockfd,(void *)packet,sizeof(tftp_packet),0,(struct sockaddr *) &client_addr, &client_len);
        
        //if acknowledgment fails re-transmit the data of same size 
        if(packet->body.ack_packet.block_number == FAILURE)
        {                
            if(mode == NORMAL || mode == OCTET)
                lseek(fd,-read_ret,SEEK_CUR);
            else if(mode == NET_ASCII)
                lseek(fd,-read_ret_for_net_ascii,SEEK_CUR);
        }
    }  
    close(fd);  //close the fd 
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    // Implement file receiving logic here
    packet->body.data_packet.block_number = 1;
    packet->body.ack_packet.block_number = 1;
    int write_ret = 1; 

    while(write_ret != 0 && write_ret != -1)
    {   
        // receive data from client
        recvfrom(sockfd,packet,sizeof(tftp_packet),0,(struct sockaddr *) &client_addr, &client_len);
    
        if(mode == OCTET)   // if octet we recieve only one bte of data so handly the write with 1 byte in packet member 
            write_ret = write(fd,packet->body.data_packet.data+0, packet->body.data_packet.block_number);
        else
            write_ret = write(fd,packet->body.data_packet.data, packet->body.data_packet.block_number);

        // validate the block umber recived and write function and update the acknowledgment packet
        if(write_ret != packet->body.data_packet.block_number )
        {
            packet->body.ack_packet.block_number = FAILURE;
        }
        else
        {
            packet->body.ack_packet.block_number = SUCCESS;
        }  
        
        // send the updated acknowledgement to client
        sendto(sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client_addr, client_len);
        
    } 
    close(fd);  //after reciveing close the fd
}

status validate_file(char filename[])
{
    //validate file present of not
    fd = open(filename,O_RDONLY);
    if(fd == -1)
    {
        if(errno == ENOENT)
        {
            //return failure if file doesn't exist
            return FAILURE;
        }
        else if(errno == EEXIST)
        {
            //return success if file exist
            return SUCCESS;
        }
    }
}