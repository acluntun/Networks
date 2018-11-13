/*
 * server.c
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

//ref: http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/



//const and globals---------------------------------------------------------------------------


#define TRUE   1
#define FALSE  0
#define PORT 3336
#define maxClients 100
#define numRooms 4
#define pSize 262174
#define msgSize 262144

static int curClientNum=0;
static int client_sock[maxClients];

static char command[1];
static char option[20];
static char lengthString[8];
static char message[262144];

//-------------------------------------------------------------------------------------------

//structs--------------------------------------------------------------------------------
struct Client {
    int socketDescriptor;
    char name[20];
    int currentRoom;
};

struct Room {
    struct Client * clientsInRoom[maxClients];
    char roomName[20];
    int roomNumber;
};

struct Client clients[maxClients];
struct Room rooms[4];

//----------------------------------------------------------------------------------


//--util methods--------------------------------------------

void sendToUser(int destSd, char* packet)
{
    printf("reciever sd: %d, packet:\n", destSd);
    int x;
    printf("command: %c, ", packet[0]);

    printf("option: ");
    for(x =1; x < 21; x++)
        printf("%c",packet[x]);
    printf(", ");

    printf("lengthString: ");
    for(x = 21; x < 29; x++)
        printf("%c",packet[x]);
    printf(", ");

    printf("message: ");
    for(x = 29; x < 262145; x++)
        printf("%c",packet[x]);
    printf("\n\n");


    if(option[20] == '\0')
    {
        option[20] = 'a';
    }


    int temp = send(destSd, packet, pSize,0);
    printf("bytes sent: %d\n", temp);
}

void initializeRooms() {
    int i;
    int r;
    for(r = 0; r < numRooms; r++) {
        for(i = 0; i < maxClients; i++) {
            rooms[r].clientsInRoom[i] = NULL;
        }
    }
    strcpy(rooms[0].roomName, "General Chat");
    rooms[0].roomNumber = 0;
    strcpy(rooms[1].roomName, "Room 1");
    rooms[1].roomNumber = 1;
    strcpy(rooms[2].roomName, "Room 2");
    rooms[2].roomNumber = 2;
    strcpy(rooms[3].roomName, "Room 3");
    rooms[3].roomNumber = 3;
    //printf("rooms initialized\n");
}


void initializeClient(int sd) {

    curClientNum++;
    int i;
    for(i = 0; i < maxClients; i++) {
        //printf("for loop\n");
        if(client_sock[i] == 0)
        {
            //printf("if statement\n");
            client_sock[i] = sd;
            clients[i].socketDescriptor = sd;
            clients[i].currentRoom = 0;
            char name[16];
            snprintf(name, 16, "%d", sd);
            strcpy(clients[i].name, "User");
            strcat(clients[i].name, name);
            //printf("3\n");
            /*
            int y=0;
            while(rooms[0].clientsInRoom[y] == NULL)
            {
                printf("while loop\n");
                rooms[0].clientsInRoom[y] = &clients[i];
                printf("asdfsdfsdf\n");
                printf("new sd: %d",rooms[0].clientsInRoom[y]->socketDescriptor);
                y++;
                break;
            }*/

            printf("Client init %d:\n sd: %d, name: %s, currentRoom: %d\n", i, clients[i].socketDescriptor, clients[i].name, clients[i].currentRoom);
            break;
        }
    }
}




int copyMessageToPacket(char * message, char * packet)
{
    //copy input into packet
    int x;
    for(x = 29; x < 262145; x++)
        packet[x] = message[x-29];
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
    /*if(option[20] == '\0')
    {
        option[20] = 'a';
    }*/
}

int clearPointer(char * pointer)
{
    int i =0;
    for(i=0; i < sizeof(pointer); i++)
        pointer[i] = '\0';
}

void getUserName(int clientSd, char** userName)
{

    int i=0;
    while(i < curClientNum)
    {
        printf("client %d sd: %d\n", i, clients[i].socketDescriptor);
        if(clients[i].socketDescriptor == clientSd)
        {
            *userName = clients[i].name;
            //printf("userName %s\n:", *userName);
            break;
        }
        i++;
    }

    //printf("userName = %s\n", *userName);
}

void getSdFromName(char* userName, int* clientSd)
{
    int i=0;
    while(i < curClientNum)
    {
        if(strcmp(clients[i].name,userName)==0)
        {
            *clientSd = clients[i].socketDescriptor;
            break;
        }
        i++;
    }

}



int breakPackage(char * input)
{

    printf("input: %s\n", input);

    clearPointer(command);
    clearPointer(option);
    clearPointer(lengthString);
    clearPointer(message);


    command[0] = input[0];

    int x;

    for(x = 29; x < 262145; x++)
        message[x-29] = input[x];
    for(x = 21; x < 29; x++)
        lengthString[x-21] = input[x];
    for(x =1; x < 21; x++)
        option[x-1] = input[x];

    printf("command: %s\n", command);
    printf("option: %s\n", option);
    printf("length: %s\n", lengthString);
    printf("msg: %s\n", message);
    return 0;
}

//-------------End util----------------------------------------------



//--send message functions---------------------------------------------

//---------------------------------------------------------------------------------

//-------------SEND FILE-----------------------------

void sendFileUser(int senderSd)
{
    char* senderName;
    getUserName(senderSd, &senderName);

    //printf("senderName = %s\n", senderName);

    char packet[pSize];
    clearPointer(packet);

    packet[0] = command[0];
    copyOptionToPacket(senderName, packet);
    copyMessageSizeToPacket(lengthString, packet);
    copyMessageToPacket(message, packet);

    int toSendSd = -1;

    getSdFromName(option, &toSendSd);
    printf("senderName: %s\n", option);
    printf("toSendSd: %d\n", toSendSd);

    sendToUser(toSendSd, packet);

}

void sendFileAll(int clientSd)
{

    char* senderName;

    getUserName(clientSd,&senderName);
    char package[pSize];
    clearPointer(package);

    clearPointer(package);
    copyOptionToPacket(senderName, package);
    copyMessageSizeToPacket(lengthString, package);
    copyMessageToPacket(message, package);
    package[0]='f';
    int x;
    for(x = 0 ; x < curClientNum; x++)
    {
        sendToUser(client_sock[x], package);
    }
}

void sendFileRoom(int clientSd)
{
    int clientCurRoom = -1;
    int i=0;



    while(i < curClientNum)
    {
        if(clients[i].socketDescriptor == clientSd)
        {
            clientCurRoom = clients[i].currentRoom;
            break;
        }
        i++;
    }

    i=0;


    int y=0;

    if(rooms[clientCurRoom].clientsInRoom[2] == NULL)
        printf("null");
    else
        printf("not null");

    while(rooms[clientCurRoom].clientsInRoom[y] != NULL)
    {
        //strcpy(option, rooms[clientCurRoom].clientsInRoom[y]->name);
        //sendFileUser(clientSd);

        char* senderName;

        getUserName(clientSd,&senderName);
        char package[pSize];

        clearPointer(package);
        copyOptionToPacket(senderName, package);
        copyMessageSizeToPacket(lengthString, package);
        copyMessageToPacket(message, package);
        package[0]='f';
        int x;

        sendToUser(rooms[clientCurRoom].clientsInRoom[y]->socketDescriptor, package);


        y++;
    }

}






void roomRemove(int sd)
{
    //printf("currentRoom: %d\n", clients[i].currentRoom);
    //struct Client client;
    //struct Room room;
    //printf("ROOM REMOVE\n");

    int roomNum;

    int i = 0;
    //printf("currentRoom: %d\n", clients[i].currentRoom);

    while(i < maxClients)
    {

        if(clients[i].socketDescriptor == sd)
        {
            //printf("1\n");
            //client = clients[i];
            printf("client %d: sd: %d, name: %s, curRoom: %d\n", i, clients[i].socketDescriptor, clients[i].name, clients[i].currentRoom);

            roomNum = clients[i].currentRoom;
            break;
        }
        i++;
    }

    //printf("2\n");
    i = 0;
    printf("roomNum: %d\n", roomNum);



    //remove client from clients list in room
    int personToDelete = 0;
    int personToMove = 0;
    while(rooms[roomNum].clientsInRoom[personToMove] != NULL)
    {
        //printf("in while personToDelete: %d\n", personToDelete);
        if(sd == rooms[roomNum].clientsInRoom[personToMove]->socketDescriptor)
        {
            //printf("in if\n");
            personToDelete = personToMove;
        }
        personToMove++;
    }
    personToMove--;
    //printf("personToDelete: %d, personToMove: %d\n", personToDelete, personToMove);
    rooms[roomNum].clientsInRoom[personToDelete] = rooms[roomNum].clientsInRoom[personToMove];
    rooms[roomNum].clientsInRoom[personToMove] = NULL;

    //printf("client sd: %d", rooms[roomNum].clientsInRoom[0]->socketDescriptor);


    //printf("FINAL\n");
}

void roomAdd(int sd, int roomNum){
    //printf("ROOM ADD\n");
    //printf("sd in: %d\n", sd);
    int i=0;
    while(i < maxClients)
    {
        //printf("sd in while: %d\n", clients[i].socketDescriptor);
        if(clients[i].socketDescriptor == sd)
        {
            clients[i].currentRoom = roomNum;
            //printf("sd in while: %d\n", clients[i].currentRoom);
            break;
        }
        i++;
    }


    int y=0;
    while(rooms[roomNum].clientsInRoom[y] != NULL)
    {

        //printf("new sd: %d\n",rooms[0].clientsInRoom[y]->socketDescriptor);
        y++;
    }

    //printf("y: %d\n", y);

    rooms[roomNum].clientsInRoom[y] = &clients[i];
    //printf("sd: %d\n", clients[i].socketDescriptor);

    //printf("END ROOM ADD %d\n", rooms[roomNum].clientsInRoom[y]->currentRoom);

}

void removeClient(int sd)
{
    int i = 0;
    while(i < curClientNum) {
        if(clients[i].socketDescriptor == sd) {

            break;
        }
        i++;
    }
    int roomNum = clients[i].currentRoom;

    if(i != curClientNum)
    {
        //remove socket
        int x=0;
        for(x= 0; x < curClientNum; x++)
        {
            if(client_sock[x] == sd)
            {
                client_sock[x] = 0;
                break;
            }

        }

        //printf("client sock %d: %d\n", x, client_sock[x]);
        //remove client from clients list
        //printf("client %d sd before: %d\n", i, clients[i].socketDescriptor);
        //printf("last client %d sd before: %d\n", curClientNum-1, clients[curClientNum-1].socketDescriptor);

        clients[i] = clients[curClientNum -1];
        curClientNum--;
        //printf("client %d sd after: %d\n", i, clients[i].socketDescriptor);
    }
}


void changeUserName (int clientSd)
{
    printf("CHANGE USER NAME\n");
    int i;
    //int roomNum;
    for(i= 0; i < curClientNum; i++)
    {
        if(clients[i].socketDescriptor == clientSd)
        {
            strcpy(clients[i].name, option);
            //roomNum = clients[i].currentRoom;
        }
        break;
    }

    //printf("roomNum: %d\n", roomNum);
    i=0;
    int y=0;

    for(y=0; y<4; y++)
    {
        for(i=0; i<maxClients; i++)
        {
            if(rooms[y].clientsInRoom[i] != NULL && rooms[y].clientsInRoom[i]->socketDescriptor == clientSd)
            {
                strcpy(rooms[y].clientsInRoom[i]->name, option);
            }
        }
    }



    char packet[pSize];
    clearPointer(packet);
    char msg[1000]= "Your username has been changed to: ";
    strcat(msg, option);
    packet[0] = '\0';
    copyOptionToPacket("SERVER", packet);

    char buff[9];
    snprintf(buff,9,"%d",strlen(msg));
    copyMessageSizeToPacket(buff,packet);

    copyMessageToPacket(msg, packet);

    sendToUser(clientSd, packet);
}

//r: send message to everyone in the requested user’s current room
void sendRoomMsg (int clientSd)
{
    int clientCurRoom = -1;
    int i=0;



    while(i < curClientNum)
    {
        if(clients[i].socketDescriptor == clientSd)
        {
            clientCurRoom = clients[i].currentRoom;
            break;
        }
        i++;
    }

    i=0;


    int y=0;

    if(rooms[clientCurRoom].clientsInRoom[2] == NULL)
        printf("null");
    else
        printf("not null");

    while(rooms[clientCurRoom].clientsInRoom[y] != NULL)
    {
        //strcpy(option, rooms[clientCurRoom].clientsInRoom[y]->name);
        //sendFileUser(clientSd);

        char* senderName;

        getUserName(clientSd,&senderName);
        char package[pSize];

        clearPointer(package);
        copyOptionToPacket(senderName, package);
        copyMessageSizeToPacket(lengthString, package);
        copyMessageToPacket(message, package);
        package[0]='\0';
        int x;

        sendToUser(rooms[clientCurRoom].clientsInRoom[y]->socketDescriptor, package);


        y++;
    }


    /*int i=0;

    struct Client client;
    for(i = 0; i < curClientNum; i++)
    {
        if(client_sock[i] == clientSd)
        {
            client = clients[i];
            break;
        }
    }

    struct Room clientRoom =  rooms[client.currentRoom];

    i = 0;
    while(i < maxClients) {
        if(clientRoom.clientsInRoom[i] != NULL)
        {
            sendToUser(clientRoom.clientsInRoom[i]->socketDescriptor, message);
        }
        i++;
    }

    int clientCurRoom = -1;
    i=0;



    while(i < curClientNum)
    {
        if(clients[i].socketDescriptor == clientSd)
        {
            clientCurRoom = clients[i].currentRoom;
            break;
        }
        i++;
    }

    i=0;


    int y=0;


    while(rooms[clientCurRoom].clientsInRoom[y] != NULL)
    {
        //strcpy(option, rooms[clientCurRoom].clientsInRoom[y]->name);
        //sendFileUser(clientSd);

        char* senderName;

        getUserName(clientSd,&senderName);
        char package[pSize];

        clearPointer(package);
        copyOptionToPacket(senderName, package);
        copyMessageSizeToPacket(lengthString, package);
        copyMessageToPacket(message, package);
        package[0]='\0';
        int x;

        sendToUser(rooms[clientCurRoom].clientsInRoom[y]->socketDescriptor, package);


        y++;
    }*/

}

void listAllUsers(int clientSd)
{
    printf("LISTALLUSERS\n");
    int i = 0;
    char toSendMsg[msgSize];
    int endStr = 0;
    char packet[pSize];
    clearPointer(packet);
    clearPointer(toSendMsg);


    printf("curClientNum: %d\n", curClientNum);
    for (i=0; i<curClientNum; i++)
    {
        strcat(toSendMsg, clients[i].name);
        strcat(toSendMsg, ",");

    }

    printf("toSendMsg: %s\n", toSendMsg);



    packet[0] = '\0';

    copyOptionToPacket("SERVER", packet);
    char buff[9];
    snprintf(buff,9,"%d",strlen(toSendMsg));
    copyMessageSizeToPacket(buff,packet);
    copyMessageToPacket(toSendMsg, packet);

    sendToUser(clientSd, packet);


}


void listUsersInRoom (int clientSd)
{


    char packet[pSize];
    char namesList[msgSize];
    clearPointer(packet);
    clearPointer(namesList);

    int roomNum;
    int i=0;
    while (i < maxClients)
    {
        if(clients[i].socketDescriptor == clientSd)
        {
            roomNum = clients[i].currentRoom;
            break;
        }
        i++;
    }

    printf("roomNum: %d\n", roomNum);

    i=0;
    clearPointer(namesList);
    while(i<maxClients)
    {

        printf("in while %d\n", i);
        if(rooms[roomNum].clientsInRoom[i] != NULL)
        {
            printf("client %d name: %s\n", i, rooms[roomNum].clientsInRoom[i]->name);
            strcat(namesList, rooms[roomNum].clientsInRoom[i]->name);
            strcat(namesList, ",");
        }
        else
            break;

        i++;
    }

    printf("namesList: %s\n", namesList);

    packet[0] = '\0';

    copyOptionToPacket("SERVER", packet);
    char buff[9];
    snprintf(buff,9,"%d",strlen(namesList));
    copyMessageSizeToPacket(buff,packet);
    copyMessageToPacket(namesList, packet);

    sendToUser(clientSd, packet);

    /*
    packet[0] = '\0';

    copyOptionToPacket("SERVER", packet);
    //printf("1\n");
    int length = strlen(namesList);
    //printf("length %d\n", length);

    snprintf(lengthString, 8, "%d", length);
    printf("                 ");
    copyMessageSizeToPacket(lengthString, packet);
    copyMessageToPacket(namesList, packet);
    //printf("Good\n");
    sendToUser(clientSd, packet);
     */


}

//END ELI’S STUFF------------------------------------------
//AREEJS STUFF--------------------------------------------

void whisper (int senderSd)
{

    char* senderName;
    getUserName(senderSd, &senderName);

    //printf("senderName = %s\n", senderName);

    char packet[pSize];
    clearPointer(packet);

    packet[0] = command[0];
    copyOptionToPacket(senderName, packet);
    copyMessageSizeToPacket(lengthString, packet);
    copyMessageToPacket(message, packet);

    int toSendSd = -1;

    getSdFromName(option, &toSendSd);
    //printf("senderName: %s\n", option);
    //printf("toSendSd: %d\n", toSendSd);

    sendToUser(toSendSd, packet);
}



//---------------------------------------

void switchRoom (int clientSd, char newRoom)
{
    char toSendStr[pSize];
    clearPointer(toSendStr);
    toSendStr[0] = '\0';
    copyOptionToPacket("SERVER", toSendStr);

    clearPointer(message);
    //clearPointer(option);
    //clearPointer(command);

    int curRoom;
    int i = 0;
    while(i < curClientNum) {
        if(clients[i].socketDescriptor == clientSd)
        {
            curRoom = clients[i].currentRoom;
            break;
        }
        i++;
    }


    int nRoom;
    if(newRoom =='X' || newRoom == 'x')
        nRoom = 1;
    else if(newRoom =='Y' || newRoom == 'y')
        nRoom = 2;
    else if(newRoom =='1')
        nRoom = 3;
    else
        nRoom = 0;



    if (nRoom == 0)
    {


        clearPointer(message);
        char roomList[6 * numRooms];
        strcat(roomList,rooms[0].roomName);
        int j;
        for (j = 2; j < numRooms; j++)
        {
            strcat(roomList, ",");
            strcat(roomList, rooms[i+1].roomName);
        }

        snprintf(lengthString, 8, "%d", sizeof(roomList));
        copyMessageSizeToPacket(lengthString, toSendStr);
        copyMessageToPacket(roomList, toSendStr);

        sendToUser(clientSd, toSendStr);


    }
    else if ( clients[i].currentRoom == nRoom)
    {
        clearPointer(message);
        strcat(message, "You are currently in this room");
        snprintf(lengthString, 8, "%d", sizeof("You are currently in this room"));
        copyMessageSizeToPacket(lengthString, toSendStr);
        copyMessageToPacket(message, toSendStr);

        sendToUser(clientSd, toSendStr);


    }
    else
    {
        clearPointer(message);
        int person;
        int noGood =0;
        for(person =0; person < maxClients; person++)
        {
            if(rooms[nRoom].clientsInRoom[person] == NULL)
            {
                noGood = 0;
                strcat(message, "successful room change");
                snprintf(lengthString, 8, "%d", sizeof("successful room change"));
                copyMessageSizeToPacket(lengthString, toSendStr);
                copyMessageToPacket(message, toSendStr);

                roomRemove(clientSd);
                roomAdd(clientSd, nRoom);

                sendToUser(clientSd, toSendStr);
                break;
            }
            noGood =1;
        }


        if(noGood)
        {
            nRoom = curRoom;
            clearPointer(message);
            strcat(message, "failed to enter room");
            snprintf(lengthString, 8, "%d", sizeof("failed to enter room"));
            copyMessageSizeToPacket(lengthString, toSendStr);
            copyMessageToPacket(message, toSendStr);
            sendToUser(clientSd, toSendStr);
        }
    }

}


//----------------------------------------------------------

void listCmds (int clientSd)
{
    char toSendStr[pSize];
    clearPointer(toSendStr);

    clearPointer(message);
    clearPointer(option);
    clearPointer(command);

    strcat(message, "b:BROADCAST,c:COMMANDS,d:DISCONNECT,e:FILE FOR EVERYONE,f:FILE FOR ONE PERSON,g:FILE FOR GROUP/ROOM,h:LIST OF USERS IN ROOM,l:LIST ALL CLIENTS,n:NAME,r:ROOM MESSAGE,s:SWITCH ROOMS,w:WHISPER");
    snprintf(lengthString, 8, "%lu", sizeof("b:BROADCAST,c:COMMANDS,d:DISCONNECT,e:FILE FOR EVERYONE,f:FILE FOR ONE PERSON,g:FILE FOR GROUP/ROOM,h:LIST OF USERS IN ROOM,l:LIST ALL CLIENTS,n:NAME,r:ROOM MESSAGE,s:SWITCH ROOMS,w:WHISPER"));


    toSendStr[0] = '\0';

    copyOptionToPacket("SERVER", toSendStr);
    copyMessageSizeToPacket(lengthString, toSendStr);
    copyMessageToPacket(message, toSendStr);

    sendToUser(clientSd, toSendStr);
}


//----------------------------------------------------------


//-------------------------------------------------------

void disconnectUser (int clientSd) {
    char toSendStr[pSize];
    clearPointer(toSendStr);
    strcpy(toSendStr, command);
    copyOptionToPacket(option, toSendStr);
    copyMessageSizeToPacket(lengthString, toSendStr);
    copyMessageToPacket(message, toSendStr);

    //printf("disconnect sd: %d\n", clients[0].socketDescriptor);
    //printf("disconnect currentRoom: %d\n", clients[0].currentRoom);
    roomRemove(clientSd);
    removeClient(clientSd);

    sendToUser(clientSd, toSendStr);

}


//END AREEJS STUFF----------------------------------------

void broadcast(int clientSd)
{

    printf("BROADCAST\n");
    //printf("currentRoom: %d\n", clients[0].currentRoom);
    char* senderName;

    getUserName(clientSd,&senderName);
    char package[pSize];

    //printf("currentRoom after getusername: %d\n", clients[0].currentRoom);
    clearPointer(package);
    //printf("currentRoom after clear: %d\n", clients[0].currentRoom);
    copyOptionToPacket(senderName, package);
    //printf("currentRoom after copy option: %d\n", clients[0].currentRoom);
    copyMessageSizeToPacket(lengthString, package);
    //printf("currentRoom after copy size: %d\n", clients[0].currentRoom);
    copyMessageToPacket(message, package);
    //printf("currentRoom after copy msg: %d\n", clients[0].currentRoom);
    package[0]='\0';
    int x;
    for(x = 0 ; x < curClientNum; x++)
    {
        sendToUser(client_sock[x], package);
    }

    //printf("currentRoom after broadcast: %d\n", clients[0].currentRoom);

}

void selectCommand(int curClientSd)
{


    switch(command[0])
    {
    case('b'): broadcast(curClientSd);
    break;
    case('c'): listCmds(curClientSd);
    break;
    case('d'): disconnectUser(curClientSd);
    break;
    case('e'): sendFileAll(curClientSd);
    break;
    case('f'): sendFileRoom(curClientSd);
    break;
    case('g'): sendFileRoom(curClientSd);
    break;
    case('h'): listUsersInRoom(curClientSd);
    break;
    case('l'): listAllUsers(curClientSd);
    break;
    case('n'): changeUserName(curClientSd);
    break;
    case('r'): sendRoomMsg(curClientSd);
    break;
    case('s'): switchRoom(curClientSd, option[0]);
    break;
    case('w'): whisper(curClientSd);
    break;
    default: listCmds(curClientSd);

    }
}



int main (int argc, char *argv[])
{
    int master_sock, client_sock[20], addrlen, new_sock, max_clients = 20, i, sd, activity;
    int max_sd;
    struct sockaddr_in address;
    char buffer[pSize];

    initializeRooms();

    fd_set readfds;
    //char *message = "Hello, this is the server";

    for (i=0; i < 20; i++)
    {
        client_sock[i] = 0;
    }
    master_sock = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(master_sock, (struct sockaddr *)&address, sizeof(address));

    listen(master_sock, 20);

    addrlen = sizeof(address);

    while(1)
    {
        FD_ZERO(&readfds);

        FD_SET(master_sock, &readfds);
        max_sd = master_sock;

        for(i=0; i< max_clients; i++)
        {
            sd = client_sock[i];
            if(sd>0)
                FD_SET(sd, &readfds);

            if(sd > max_sd)
                max_sd = sd;

        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        //new connection comes to master socket
        if(FD_ISSET(master_sock, &readfds))
        {
            new_sock = accept(master_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);

            printf("New Connection: sockfd %d, ip: %s\n", new_sock, inet_ntoa(address.sin_addr));

            //send(new_sock, message, strlen(message), 0);

            for(i=0; i<max_clients; i++)
            {
                if(client_sock[i] ==0)
                {
                    client_sock[i] = new_sock;
                    initializeClient(new_sock);
                    roomAdd(new_sock,0);
                    break;
                }
            }

        }

        //check for activity on client sockets
        for(i=0; i<max_clients; i++)
        {
            sd = client_sock[i];
            int readResult = 0;


            if(FD_ISSET(sd,&readfds))
            {
                int numBytesRead = 0;
                char packet[pSize];
                //printf("sd %d\n", sd);
                if((readResult = recv(sd, buffer, pSize, 0)) == 0)
                {
                    //printf("readResult if: %d\n", readResult);
                    //printf("if\n");

                    //printf("Client disconnected: ip %s", inet_ntoa(address.sin_addr));
                    roomRemove(sd);
                    removeClient(sd);

                    close(sd);

                    client_sock[i] = 0;
                }
                else
                {
                    //printf("else\n");
                    //printf("readResult %d\n", readResult);
                    int done = 0;
                    //buffer[readResult]='\0';
                    numBytesRead = readResult;

                    int i=0;
                    for(i=0; i<numBytesRead; i++)
                    {
                        packet[i] = buffer[i];
                    }

                    while(numBytesRead < pSize-1 && readResult > 0)
                    {
                        readResult = recv(sd, buffer, pSize, 0);
                        //printf("while readResult: %d\n", readResult);
                        //printf("while numBytesRead before: %d\n", numBytesRead);

                        //buffer[readResult]='\0';
                        for(i=0; i<readResult; i++)
                        {
                            //printf("while for readResult: %d\n", readResult);
                            packet[(numBytesRead-1)+i] = buffer[i];
                            //printf("%c", packet[(numBytesRead-1)+i]);
                        }
                        numBytesRead += readResult;

                        //printf("while numBytesRead after: %d\n", numBytesRead);

                    }



                    /*DEBUG

                    int x;
                    printf("command: %c, ", packet[0]);

                    printf("option: ");
                    for(x =1; x < 21; x++)
                    {
                        if(packet[x]!='\0')
                        {
                            printf("%c",packet[x]);
                        }
                        else {
                            break;
                        }
                    }

                    printf(", ");

                    printf("lengthString: ");

                    for(x = 21; x < 29; x++)
                    {
                        if(packet[x]!='\0')
                        {
                            printf("%c",packet[x]);
                        }
                        else {
                            break;
                        }
                    }
                    printf(", ");

                    printf("message: ");
                    for(x = 29; x < 262145; x++)
                    {
                        if(packet[x]!='\0'){
                            printf("%c",packet[x]);
                        }
                        else {
                            break;
                        }
                    }

                    printf("\n\n");
                    printf("END PARSE INCOMING MSG\n");

                    for(x = 0; x < pSize; x++)
                    {
                        packet[x]!='\0';

                    }*/


                    clearPointer(command);
                    clearPointer(option);
                    clearPointer(lengthString);
                    clearPointer(message);

                    breakPackage(packet);
                    selectCommand(sd);

                }
            }
        }


    }

}

//end untested------------------------------------------------------------------




