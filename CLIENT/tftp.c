/* Common file for server & client */

#include "tftp.h"
#include "tftp_client.h"
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
void send_file(tftp_client_t *client, tftp_packet *packet) 
{
    // Implement file sending logic here
    packet->body.data_packet.block_number = 1;  //block number intillailly 1
    int read_ret = 1;
    
    int Net_ascii_byte_count = 0; 
    int read_ret_for_net_ascii = 1;
    int net_Ascii_flag = 0;
    char ch;
    while( read_ret != 0 )
    {
        if(mode == NORMAL)
        {
            read_ret = read(fd, packet->body.data_packet.data ,512);
        }
        else if(mode == OCTET)
        {
            read_ret = read(fd, packet->body.data_packet.data ,1);
        }
        else if(mode == NET_ASCII)
        {
            while((read_ret_for_net_ascii = read(fd, &ch ,1)) > 0)
            {
                //now check character is \n or not 
                if(ch == '\n' && Net_ascii_byte_count <= 510 )
                {
                    packet->body.data_packet.data[Net_ascii_byte_count++] = '\r';       //insert \r 
                    packet->body.data_packet.data[Net_ascii_byte_count++] = '\n';       //insert \n
                }
                else if(ch != '\n' && Net_ascii_byte_count < 512)
                {
                    //add the character to packet and increment the byte count
                    packet->body.data_packet.data[Net_ascii_byte_count++] = ch;
                }
                else
                {
                    break; // break to send the data and to handle edge case
                }

                if(Net_ascii_byte_count == 512)
                {
                    break;
                }
            }
            //when file offset reach EOF and already some bytes of data is added to packet
            if(read_ret_for_net_ascii <= 0  && Net_ascii_byte_count != 0)    //when read failed read end of file
            {
                //update count value to data packet->block number with left out chunk less then or equal to 512 and send
                packet->body.data_packet.block_number = Net_ascii_byte_count;
                sendto(client->sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);
                
                //receieve the acknowledgement from server
                recvfrom(client->sockfd,(void *)packet,sizeof(tftp_packet),0,(struct sockaddr *) &client->server_addr, &client->server_len);
                net_Ascii_flag = 1; //flag will be set when read ret becomes zero , indicating EOF so transmits 
                read_ret = 0;       //read ret is made to zero so it terminates the outer loop when file reached EOF
                Net_ascii_byte_count = 0;   //block number should be updated with 0 for upcoming send to so server termeniates 
            }
            //break the loop only when both are zero i.e  for empty file case
            else if(read_ret_for_net_ascii == 0  && Net_ascii_byte_count == 0)
            {
                read_ret = 0;
            }
        }
        else
        {
            printf("Invalid Mode\n");   //handling Invalid mode edge case
            return;
        }
    
        
        //handling send function for packet data filled exactly with 512 and reached EOF [end of file] to send block number updated with zero (0)
        if (mode == NET_ASCII && (net_Ascii_flag == 1 || Net_ascii_byte_count == 512))
        {
            // update the byte count and send the data
            packet->body.data_packet.block_number = Net_ascii_byte_count;
            sendto(client->sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);
            
            //reset after sending the data for next chunk of data
            net_Ascii_flag = 0;
            Net_ascii_byte_count = 0;
        }
        else 
        {
            packet->body.data_packet.block_number = read_ret;
            sendto(client->sockfd,packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);
        }

        //recived acknowledgment from server
        recvfrom(client->sockfd,(void *)packet,sizeof(tftp_packet),0,(struct sockaddr *) &client->server_addr, &client->server_len);
        
        //validate the acknowledgment
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

void receive_file(tftp_client_t *client, tftp_packet *packet) 
{
    // Implement file receiving logic here
    packet->body.data_packet.block_number = 1;
    packet->body.ack_packet.block_number = 1;
    int write_ret = 1; 

    while( write_ret != 0 && write_ret != -1)
    {   
      
        //receieve file content from server
        recvfrom(client->sockfd,packet,sizeof(tftp_packet),0,(struct sockaddr *) &client->server_addr, &client->server_len);
       
        if(mode == OCTET) //if octet handle only the on1 byte of data packet member
            write_ret = write(fd,packet->body.data_packet.data+0, packet->body.data_packet.block_number);
        else
            write_ret = write(fd,packet->body.data_packet.data, packet->body.data_packet.block_number);
        
        //validate the bytes and update acknowledgemnt packet
        if(write_ret != packet->body.data_packet.block_number )
        {
            packet->body.ack_packet.block_number = FAILURE;
        }
        else
        {
            packet->body.ack_packet.block_number = SUCCESS;
        } 
        
        //send acknowledgemnt to the server 
        sendto(client->sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);
    } 
    close(fd);  //close the fd
}