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

    ** Client Part **
    client works/does operations like read or write to files based on user opted MODE
*/


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

MODE mode = NORMAL;
int connect_flag;
int fd;
int main() 
{
    char command[256];
    tftp_client_t client; 
    tftp_packet packet;
    memset(&client, 0, sizeof(client));  // Initialize client structure
    packet.body.request.mode = NORMAL;      //initiallise mode defaultly as NORMAL
    
    // Main loop for command-line interface
    while (1) 
    {
        printf("\n");
        printf("1.Connect\n");
        printf("2.Get\n");
        printf("3.Put\n");
        printf("4.Mode\n");
        printf("5.Quit\n");
        printf("Choose an option: ");
        packet.body.request.mode = mode;    //initiallze mode every time so get and put can be handled
        int option;
        scanf("%d",&option);
        
        if(option < 1 && option > 5)
        {
            printf("Invalid option\n");
            continue;
        }
        
        if((option == 2 || option == 3) && connect_flag == 0)
        {
            printf("Before get or put first connect to server\n");
            continue;
        }
        
        switch(option)
        {
            case 1:
                {
                    char ip[25]; // 0.0.0.0  -  255.255.255.255
                    printf("Enter Server IP Address: ");
                    scanf(" %[^\n]",ip);
                    if(ipv4_validation(ip) == SUCCESS)
                    {
                        connect_to_server(&client,ip,0);
                        printf("Entered  IP is valid, connection to server was Successfull..!\n");
                        connect_flag = 1;
                    }
                    else
                    {
                        printf("Entered IP is In-valid, Connecting to server Failed..!\n");
                    }
                }
                break;
            case 2:
                //get
                get_file(&client,&packet);
                break;
            case 3:
                //put
                put_file(&client,&packet);
                break;
            case 4:
                //select mode
                select_mode();
                break;
            case 5:
                //disconnect the socket and exit the client
                disconnect(&client);
                exit(0);
                break;
            default:
                printf("Invalid Option\n");
        }
        /* 
            //another method to read the input rather then scanf
            //this method is used in real time applications as scanf can be easily hacked via making the stdin buffer full
            // so in real time application this method is used 
            
            printf("Enter file name: ");
            fgets(command, sizeof(command), stdin);

            // Remove newline character
            command[strcspn(command, "\n")] = 0;       

            printf("%s\n",command);
        */
    }
    return 0;
}

//validate an ip
status ipv4_validation(char ip[])
{
    //handling ip starting  and ending with  dot (.) character
    if(ip[0] == '.')
    {
       //printf("failed due to starting  . \n");
        return FAILURE;
    }
    else if(ip[strlen(ip) - 1] == '.' || ip[strlen(ip) - 1] == ' ')
    {
       //printf("failed due to last charcater\n");
        return FAILURE;
    }
    
    int i=0;
    char token[25];
    int token_index = 0;
    int no_of_dots = 0;
    while(ip[i] != '\0')
    {
        //handling ip havinng space character
        if(ip[i] == ' ')
        {
            //printf("failed due to space\n");
            return FAILURE;
        }
        //handling ip having consequtive dot characters
        else if(ip[i+1] != '\0' && ip[i] == '.' && ip[i+1] == '.')
        {
            //printf("failed due to consequtive dots\n");
            return FAILURE;
        }
        
        //handling chacters with in 0 - 9 to find decimal value
        if(ip[i] >= '0' && ip[i] <= '9')
        {
            token[token_index++] = ip[i];
        }
        else if(ip[i] == '.')
        {
            //handling dots to get 4 proper fields
            no_of_dots++;
            
            token[token_index] = '\0';
            if( atoi(token) < 0 ||  atoi(token) > 255)  //handling range of ip address fields
            {
                //printf("%d\n",atoi(token));
                //printf("failed due to invalid range\n");
                return FAILURE;
            }
            token_index = 0;
        }
        else
        {
            //printf("failed due to missmatch character\n");
            return FAILURE;
        }
        i++;
    }

    if(no_of_dots != 3)
    {
        //printf("failed due to no 3 dots\n");
        return FAILURE;
    }

    //validate the last token
    token[token_index] = '\0';
    if( atoi(token) < 0 ||  atoi(token) > 255)
        return FAILURE;

    //valid
    return SUCCESS;
}

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port) 
{
    memset(client,0,sizeof(client));
    // Create UDP socket
    /* Create a client socket */
	if((client->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Error: Could not create socket\n");
		return;
	} 

    // Set socket timeout option


    // Set up server address
    client->server_addr.sin_family = AF_INET;
	client->server_addr.sin_port = htons(5001); 
	client->server_addr.sin_addr.s_addr = inet_addr(ip);

    //update ip address to client 
    strcpy(client->server_ip,ip);

    client->server_len = sizeof(struct sockaddr_in);
}

void select_mode()
{
    printf("  MODES  \n");
    printf("1.Normal\n");
    printf("2.Octet\n");
    printf("3.Net-ASCII\n");
    printf("4.Exit\n");
    printf("Choose Mode: ");
    int option;
    scanf("%d",&option);
    if(option < 1 && option >4)
    {
        printf("Incorrect option\n");
    }
    else
    {
        switch(option)
        {
            case NORMAL:
                mode = NORMAL;
                //packet.body.request.mode = NORMAL;
                printf("Mode is set to : \"NORMAL\"\n");
                break;
            case OCTET:
                mode = OCTET;
                //packet.body.request.mode = OCTET;
                printf("Mode is set to : \"OCTET\"\n");
                break;
            case NET_ASCII:
                mode = NET_ASCII;
                //packet.body.request.mode = NET_ASCII;
                printf("Mode is set to : \"NET_ASCII\"\n");
                break;
            default:        //if directly exiting whithout choosing mode set to NORMAL
               // packet.body.request.mode = NORMAL;
                mode = NORMAL;
                printf("Considering Mode as \"NORMAL\"\n");
                break;
        }
    }
}

status validate_file(char filename[])
{
    //validate file
    fd = open(filename,O_RDONLY);
    if(fd == -1)
    {
        if(errno == ENOENT)
        {
            //file doesn't present
            return FAILURE;
        }
        else if(errno == EEXIST)
        {
            //file already present
            return SUCCESS;
        }
    }
}

void put_file(tftp_client_t *client, tftp_packet *packet) 
{
    memset(packet,0,sizeof(packet));
    // read file name and validate
    char filename[20];
    printf("Enter File name: ");
    scanf(" %[^\n]",filename);

    if(validate_file(filename) == FAILURE)
    {
        printf("File is not present in Current Working Directory, Try again!\n");
        return;
    }

    // Send WRQ request and send file
    packet->opcode = WRQ;       //update opcode as WRQ so server receives
    strcpy(packet->body.request.filename,filename); //also update the filename

    //send => request to client
    int ret = sendto(client->sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);
     

     recvfrom(client->sockfd, packet,sizeof(tftp_packet),0,(struct sockaddr *) &client->server_addr, &client->server_len);

     //validate for acknowledgment
    if(packet->body.ack_packet.block_number == FAILURE)
    {
        return;
    } 
    else
    {
        //send the file
        printf("Sending File .. .. ..\n");
        send_file(client, packet); 
        printf("File Sent...!\n");
    }
}

void get_file(tftp_client_t *client, tftp_packet *packet) 
{
    // Send RRQ and recive file 
    memset(packet,0,sizeof(packet));
    // read file name and validate
    char filename[20];
    printf("Enter File name: ");
    scanf(" %[^\n]",filename);
    
    int flag_for_file = 0;
    // //validate file
    fd = open(filename,O_RDONLY);
    //if file is present, close the already open fd and open it in write only mode and also clear the content of the existing file
    if(fd > 0)
    {
        //if file alredy present close the fd 
        close(fd);
        //open in write only and clear the file content
        fd = open(filename, O_WRONLY | O_TRUNC);
    }
   
    packet->opcode = RRQ;       //update opcode as RRQ so server resends
    strcpy(packet->body.request.filename,filename); //also update the filename

    //tranmist request packet
    sendto(client->sockfd, packet, sizeof(tftp_packet), 0,(struct sockaddr *)&client->server_addr, client->server_len);   


    //get acknowledgement from the server 
    recvfrom(client->sockfd, packet,sizeof(tftp_packet),0,(struct sockaddr *) &client->server_addr, &client->server_len);

    //if acknowledgement fails, return to main while(1) as no file with requested file name present on server side
    if(packet->body.ack_packet.block_number == FAILURE)
    {
        printf("Fetching Requested File Failed from Server\n");
        return;
    }
    else
    {
        //if fd failed in read only mode i.e file is not prent
        //now acknowlegment from server is true so create a file with  same file name in write only mode 
        if(fd == -1)
        {
            //if file alredy present close the fd 
            close(fd);
            //open in write only and clear the file content
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
        }
        //receive file
        printf("Receiving File .. .. ..\n");
        receive_file(client, packet);
        printf("File Received...!\n");
    }
}

void disconnect(tftp_client_t *client) 
{
    // close fd
    close(client->sockfd);
}
