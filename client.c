/*
 * client.c
 *
 *  Created on: Feb 28, 2017
 *      Author: tnguyen44, Eli, Nick, Areej
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <assert.h>
#include <pthread.h>

#define PORT 3337
#define PackageSize 262174
char * SERVERIP= "10.102.134.225";


int copyMessageToPacket(char * message, char * packet)
{
    //copy input into packet
    int x;

    for(x = 29; x < 262145; x++)
    {
        packet[x] = message[x-29];
        
    }
}

int copyMessageSizeToPacket(char * inputSize, char * packet)
{
    int x;
    for(x = 21; x < 29; x++)
        packet[x] = inputSize[x-21];
}


int copyOptionToPacket(char * option, char * packet)
{
    int x;
    for(x =1; x < 21; x++)
        packet[x] = option[x-1];
    if(option[20] == '\0')
    {
        //option[20] = 'a';
    }
}

int clearPointer(char * pointer)
{
    int i =0;
    for(i=0; i < sizeof(pointer); i++)
            pointer[i] = '\0';
}

int readFileToBuffer(char *fileBuff, char * filePath)
{
    FILE * file = fopen(filePath, "r");
    clearPointer(fileBuff);            
    int c =0;
    if(file)
    {
        int index =0;
        while((c = getc(file)) != EOF)
        {
            fileBuff[index] = c;
            index++;
        }
        fclose(file);
    }
    else
    {
    printf("File not found\n");
    }
    int i =0;
    while(fileBuff[i] != '\0')
    {
        i++;
    }    
    return i;                


}


void* reading(void * socket)
{
    int sock = *((int *)socket);
    char command;
    char option[21];
    char sizeString[9];
    int size;
    char content [262145];

    char buff;
    while(1)
    {
           
        clearPointer(option);
        clearPointer(sizeString);
        clearPointer(content);
        
        //read 1 byte at a time til you hit the max
           int x = 0;
        while(x < 29)
        {
            read(sock, &buff, 1);
            if(x == 0)
            {
                command = buff;
            }
            else if( 1 <= x && 21 > x)
                option[x-1] = buff;
            else if( 21 <= x && 29 > x)
            {
                sizeString[x - 21] = buff;
            }

            x++;
        }
        size = atoi(sizeString);

        x=0;
        while(x < size)
        {
            read(sock, &buff, 1);
            content[x] = buff;
            x++;
        }
        int totalSize = PackageSize - 30 - size;
        x =0;
        char trash;
        while(x < totalSize)
        {
            read(sock, &trash, 1)    ;
            x++;
        }
        //client got file
        
        if(command == 'f')
        {
            FILE * file = fopen("unknown.txt", "w+");
            //if(file != NULL)
            for(x =0; x < size; x++)
            {
                fprintf(file, "%c", content[x]);
            }
            
            
            fclose(file);
            printf("%s sent a file now saved as unknown.txt\n", option);
        }
        
        //disconnet
        else if(command == 'd')
        {
            break;
        }
        
        //print stuff
        else if(command == '\0')
        {
            printf("%s: ", option);
             for(x =0; x < size; x++)
            {
                printf( "%c", content[x]);
            }
            printf("\n");
            
        }
        //wisper
        else if(command == 'w')
        {
            printf("(private message) %s: " , option);
            for(x =0; x < size; x++)
            {
                printf( "%c", content[x]);
            }
            printf("\n");
        }
        else
        {
            printf("no matching flag\n");
        }
        read(sock, content, PackageSize - 29 - size);
        clearPointer(content);

        
    }

}



int main (int argc, char *argv[])
{
    
    //connect to server
    int sockfd,n;    
    struct sockaddr_in servaddr;
    
    sockfd= socket(AF_INET, SOCK_STREAM,0);    

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    
    //inet_pton(AF_INET, SERVERIP, &(servaddr.sin_addr));
    
    servaddr.sin_addr.s_addr = inet_addr(SERVERIP);
    
    if(-1 == connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)))
    {
    printf("failed to connect\n");
    exit(0);
    }
    else
    {
        printf("connected\n");
    }
    
    //intitate the readThread
    pthread_t readThread;
    pthread_create(&readThread, NULL, reading, &sockfd);
    
    
    //writing to the server
    while(1)
    {
        char input[262145];
        char package[262173];
        
        clearPointer(input);
        clearPointer(package);
            
        fgets(input, 262145, stdin);
        
        //look for flags
        
        //found flag
        if('/' == input[0])
        {
            //get the input and input size
            char message[262142];
            
            clearPointer(message);
            
            strncpy(message,input+3,262142);
            
            int size =0;
            while(message[size] != '\0')
            {
                size++;
            }
            size--;
            message[size] = '\0';
            
            
            char inputSize[9];
            snprintf(inputSize, 8, "%d", size);
            copyMessageSizeToPacket(inputSize, package);
            
            
            //flag to see if valid
            int valid =1;
            int dc =0;
            
            //figure which flag is
            char com = input[1];
            
            package[0] = com;
            
            //serverwide input
            if( 'b' == com)
            {
                copyMessageToPacket(message, package);
            }
            
            //commands
            else if('c' == com || 'h'  ==com || 'l' ==com)
           {
               clearPointer(package);
               package[0] = com;
           }

            //dissconnect
            else if('d' == com)
            {
                write(sockfd, package, strlen(package));
                break;
            }
            //world file
            else if('e' == com)
            {
                
                char fileBuff[262145];
                clearPointer(fileBuff);
                int size = readFileToBuffer(fileBuff, message);
                char inputSize[9];
                snprintf(inputSize, 9, "%d", size);
                
                copyMessageSizeToPacket(inputSize, package);
                copyMessageToPacket(fileBuff, package);
                
                
            }
            //wisper file
            else if('f' == com)
            {
            
                char fileBuff[262145];
                clearPointer(fileBuff);
                
                //parse out username
                char username[20];
                clearPointer(username);
                
                
                const char splitOn[2] = " ";
                char *token;
                token = strtok(message, splitOn);
                
                strcpy(username, token);
                
                token = strtok(NULL, splitOn);
                
                char filename[1000];
                clearPointer(filename);
                
                strcpy(filename, token);
                int size = readFileToBuffer(fileBuff, filename);
                char inputSize[9];
                snprintf(inputSize, 9, "%d", size);

            
                copyOptionToPacket(username, package);
                copyMessageSizeToPacket(inputSize, package);
                        copyMessageToPacket(fileBuff,package);    
            
            }
            //room file
            else if('g' == com)
            {
                char fileBuff[262145];
                clearPointer(fileBuff);
                int size = readFileToBuffer(fileBuff, message);
                char inputSize[9];
                snprintf(inputSize, 9, "%d", size);
                
                copyMessageSizeToPacket(inputSize, package);    
                copyMessageToPacket(fileBuff, package);
                
            }

            //update client name
            else if('n' == com)
            {
                copyOptionToPacket(message, package);
            }
            //room input
            else if('r' == com)
            {
                copyMessageSizeToPacket(inputSize, package);
                copyMessageToPacket(message, package);
            }
            //change room
            else if('s' == com)
            {
                clearPointer(package);
               package[0] = com;
               copyOptionToPacket(message, package);

            }
            
            //wisper
            else if('w' == com)
            {
                //parse out username
               char username[20];
               clearPointer(username);


               const char splitOn[2] = " ";
               char *token;
               token = strtok(message, splitOn);

               strcpy(username, token);

               int len = strlen(username);

               strncpy(message,input+len+3,262142);
               len = strlen(message);
               snprintf(inputSize, 9, "%d", len);

               copyOptionToPacket(username, package);
               copyMessageSizeToPacket(inputSize, package);
               copyMessageToPacket(token, package);
            }
            //invalid flag
            else
            {
                printf("command not valid\n");
                valid =0;
            }
            if(valid)
            {
                /*int g;
                for(g =0; g < 100; g++)
                {
                    printf("%d: %c\n", g, package[g]);
                }*/
                
                
                send(sockfd, package, sizeof(package),0);
                //printf("%d", sizeof(package));
            }
        }//end found flag
        //no flag
        else
        {
            printf("no command not valid\n");
        }
    }
}








