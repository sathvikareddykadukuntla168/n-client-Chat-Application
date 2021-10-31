// created client.cpp
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <bits/stdc++.h>
#include <string>
#include "parsers.h"
#include <limits.h>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <sys/stat.h>

using namespace std;


#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#if __cplusplus >= 201703L && __has_include(<filesystem>)
#include <filesystem>
     namespace fs = filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
     namespace fs = experimental::filesystem;
#elif __has_include(<boost/filesystem.hpp>)
#include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#endif
#endif

#define BUF_SIZE 512
#define SLEEP_TIME 30000
void receive(int sock);
void check_connection(int sock);
pair<string,int> receivefile(int sock, char buffer[]);
mutex send_mutex;
int room;
mutex room_mutex;
string username;

int main(int argc, char *argv[]) {
    
    if(argc != 4) {
        printf("Invalid argument number\n");
        return 1;
    }
    // arg1 => ./clientmsg
    // arg2 =>  ip
    // arg3 =>  port
    // arg4  => username

    // auto input_vector = parseByDelimeter(string(argv[1]), ":", 2);
    // if(input_vector.size() != 2) {
    //     perror("Invalid input");
    //     exit(1);
    // }
    int sockfd;
    int portno = 5000;
    string ip = "127.0.0.1";
    //ip = input_vector[0];
    //portno = atoi(input_vector[1].c_str());
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[512];
    server = gethostbyname(ip.c_str());
    
    vector<string> response_headers(7);
    string name = argv[3];

    char uname[50];
    username=name;
    strcpy(uname, name.c_str());
    mkdir(uname, 0777);

    room_mutex.lock();
    room = atoi(argv[2]);

    room_mutex.unlock();
    bool  userConnected = false;
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    // TODO : create socket and get file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Unable to open a socket");
        exit(1);
    }
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }
    // TODO : connect to server with server address which is set above (serv_addr)
    socklen_t servlen = sizeof(serv_addr);
    if(connect(sockfd, (struct sockaddr*) &serv_addr, servlen) == -1) {
        perror("Unable to connect to the server");
        exit(1);
    }
    // TODO : inside this while loop, implement communicating with read/write or send/recv function
    thread receiver;
    thread connection_checker;
    thread filereceiver;
    std::cout<<" COMMANDS ::\n /rooms \n /list \n /join \n /quit \n u1,u2 : msg \n All : msg \n u1 : file*filename \n";

    while (1) {
        /* Headers mapping
         0 - Type
         1 - Who
         2 - Name
         3 - Room
         4 - Receivers
         5 - SleepTime
         6 - Content
         */
        if(userConnected == false) {
            response_headers[0] = "Connect";
            response_headers[1] = "Client";
            response_headers[2] = name;
            room_mutex.lock();
            response_headers[3] = to_string(room);
            room_mutex.unlock();
            response_headers[4] = "";
            response_headers[5] = "0";
            response_headers[6] = "";
            userConnected = true;
            receiver = thread(receive, sockfd);
            connection_checker = thread(check_connection, sockfd);
            //filereceiver=thread(receivefile,sockfd);
        }
        else {
            bzero(buffer,BUF_SIZE);
            string input;
            getline(cin,input);
            if(input.size() == 0) {
                printf("Invalid input. Please try again.\n");
                continue;
            }
            if(input[0] == '/') {
                if(input.find("/list") != string::npos) {
                    response_headers[0] = "GetUsers";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = "";
                }
                else if(input.find("/rooms")!=string::npos){
                    response_headers[0] = "GetRooms";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = "";
                }
                else if(input.find("/join") != string::npos) {
                    vector<string> splitted = parseByDelimeter(input, " ", INT_MAX);
                    if(splitted.size() != 2) {
                        printf("Invalid input. Please try again.\n");
                        continue;
                    }
                    response_headers[0] = "ChangeRoom";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = splitted[1];
                }
                else if(input.find("/quit") != string::npos) {
                    response_headers[0] = "Disconnect";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = "";
                }
                else {
                    printf("Invalid input. Please try again.\n");
                    continue;
                }
            }
            else {
                vector<string> splitted = parseByDelimeter(input, " : ", INT_MAX);
                if(splitted.size() != 2) {
                    printf("Invalid input(1). Please try again.\n");
                    continue;
                }
                    string contentOfMessage = splitted[1];
                    //tobedone:removespaces from input userslist
                    vector<string> receiverWithTime = parseByDelimeter(splitted[0], ",", INT_MAX);
                    vector<string> receivers;
                    vector<string> times;
                    bool input_valid = true;
                    for(auto i = 0; i < receiverWithTime.size(); i++) {
                        splitted = parseByDelimeter(receiverWithTime[i], "#", INT_MAX);
                        if(splitted.size() > 2 || splitted.size() < 1) {
                            printf("Invalid input(2). Please try again.\n");
                            input_valid = false;
                            break;
                        }
                        receivers.push_back(splitted[0]);
                        if(splitted.size() == 2)
                            times.push_back(splitted[1]);
                        else {
                            times.push_back("0");
                        }
                    }
                    if(!input_valid)
                        continue;
                vector<string> typeOfContent = parseByDelimeter(contentOfMessage, "*", INT_MAX);//file*filename.png
                string receiversZipped = zipArray(receivers);
                string timesZipped = zipArray(times);
                if(typeOfContent.size()!=2){
                    response_headers[0] = "Message";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = receiversZipped;
                    response_headers[5] = timesZipped;
                    response_headers[6] = contentOfMessage;
                }
                else{
                    //filesend 
                    //usernames : file*filename.vhv
                    if(typeOfContent[0]!="file"){
                        printf("Invalid input(3). Please try again.\n");
                            input_valid = false;
                            break;
                    }
                    string filename = typeOfContent[1];
                    if (!fs::exists("./" + filename))
                    {
                        cout << "Entered filename doesn't exist. Retry operation !!" << endl;
                        continue;
                    }
                    ifstream filesizeptr("./" + filename, ios::binary);
                    filesizeptr.seekg(0, ios::end);

                    int filesize = filesizeptr.tellg();
                    string filedetails=filename+"$"+to_string(filesize);
                    // inform client about the file details and wait for a response
                    response_headers[0] = "File";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = receiversZipped;
                    response_headers[5] = timesZipped;
                    response_headers[6] = filedetails;
                    
                    string response = constructMessage(response_headers);

                    send_mutex.lock();
                    char name_buffer[512];
                    memset(name_buffer,'\0',BUF_SIZE);
                    for(int i = 0 ; i < BUF_SIZE ; ++i){
                        name_buffer[i] = response[i];
                    }
                    int send_status = (int)send(sockfd, name_buffer, BUF_SIZE, 0);
                    if( send_status <= 0) {
                        perror("Error in sending the data chunk to the server.\n");
                        exit(1);
                    }
                    usleep(SLEEP_TIME);
                    send_mutex.unlock();
                    
                    cout<<"filesize sent: File details sent : "<<filesize<<endl;

                    ifstream fin("./" + filename, ifstream::binary);
                    // buffer for the file data
                    int size_transferred = 0;
                    // int sleeptime = 100000 * 20; //2.0 seconds
                    send_mutex.lock();

                    int siz = BUF_SIZE - (3 + receiversZipped.size()+to_string(room).size());
                    
                    while (size_transferred < filesize)
                    {
                        // usleep(sleeptime);       
                        char send_buffer[512];
                        string init = "#"+receiversZipped+"$"+to_string(room)+"~";
                        siz=BUF_SIZE-init.size();
                        memset(buffer, '\0', BUF_SIZE);
                        fin.read(buffer, siz);
                        streamsize dataSize = fin.gcount();


                        int ind = 0;
                        for(int i = 0; i < init.size() ; ++i){
                            send_buffer[ind++] = init[i];
                        }   
                        
                        // string buf;
                        // strcpy(buf, buffer.c_str());                     
                                  

                        for(int i = 0 ; i < siz ; ++i){
                            send_buffer[ind++] = buffer[i];
                        }
                        //cout<<"COMM : Sending data "<<send_buffer<<endl;
                        if ((int)send(sockfd, send_buffer, BUF_SIZE, 0)<0)
                        {
                            cout << "Error in sending the data chunk to the server." << endl;
                        }
                        size_transferred += siz;
                        //usleep(SLEEP_TIME);
                    }
                    send_mutex.unlock();
                    cout << "File sent successfully !!" << endl;
                    continue;
                }
            }
        }
        string response = constructMessage(response_headers);
        send_mutex.lock();
        int send_status = (int)send(sockfd, response.c_str(), response.size(), 0);
        if( send_status <= 0) {
            perror("Error on sending. Terminating the connection\n");
            exit(1);
        }
        usleep(SLEEP_TIME);
        send_mutex.unlock();
    }
    return 0;
}

void receive(int sock) {
    char buffer[BUF_SIZE];
    string file_name; 
    int file_size;
    ofstream write_to_file;

    while(1) {
        bzero(buffer,BUF_SIZE);
        int stat = (int)recv(sock, buffer, BUF_SIZE, 0);
        if(stat == 0) {
            perror("Server is down or timeout");
            exit(1);
        }
        else if(stat < 0) {
            perror("Error on receiving");
            exit(1);
        }
        string message(buffer);

        if(message[0] == '#'){
            char write_buffer[511];
            memset(write_buffer,'\0',511);
            for(int i = 1 ; i < BUF_SIZE ; ++i){
                write_buffer[i-1] = buffer[i];
            }

            string data(write_buffer); 
            ofstream fileOUT("./"+username+"/"+file_name, ios_base::binary | ios_base::out | ios_base::app); // ios_base::app
            fileOUT << data ;
            fileOUT.close();

            continue;
        }
        vector<string> headers = parse(message);
        if(headers[0] == "Connect") {
            string room = headers[getHeaderIndex("Room")];
            string name = headers[getHeaderIndex("Name")];
            cout << "Hello " << name << "! " << "This is room #" << room << "\n";
        }
        else if(headers[0] == "Disconnect") {
            string room = headers[getHeaderIndex("Room")];
            string name = headers[getHeaderIndex("Name")];
            cout << "Good Bye " << name << "\n";
            exit(1);
        }
        else if(headers[0] == "GetUsers") {
            string room = headers[getHeaderIndex("Room")];
            string name = headers[getHeaderIndex("Name")];
            vector<string> users = parseByDelimeter(headers[getHeaderIndex("Content")], "#", INT_MAX);
            cout << "This is list of users in room # " << room << "\n";
            for(auto i = 0; i < users.size(); i++) {
                cout << i+1 << ". " << users[i] << "\n";
            }
        }
        else if(headers[0]=="GetRooms"){
            string name = headers[getHeaderIndex("Room")];
            vector<string> rooms = parseByDelimeter(headers[getHeaderIndex("Content")], "#", INT_MAX);
            cout << "This is list of rooms # \n";
            for(auto i = 0; i < rooms.size(); i++) {
                cout << i+1 << ". " << rooms[i] << "\n";
            }
        }
        else if(headers[0] == "ChangeRoom") {
            string oldRoom = headers[getHeaderIndex("Room")];
            string newRoom = headers[getHeaderIndex("Content")];
            string name = headers[getHeaderIndex("Name")];
            string rec_status = headers[getHeaderIndex("Receivers")];  // Both / Join / Left
            if(rec_status == "Both") {
                cout << name << " left room #" << oldRoom << "\n";
                cout << name << " joined room #" << newRoom << "\n";
                room_mutex.lock();
                room = atoi(newRoom.c_str());
                room_mutex.unlock();
            }
            else if(rec_status == "Left") {
                cout << name << " left room #" << oldRoom << "\n";
            }
            else if(rec_status == "Join") {
                cout << name << " joined room #" << newRoom << "\n";
            }
        }
        else if(headers[0] == "Error") {
            string error = headers[getHeaderIndex("Content")];
            cout << error << "\n";
        }
        else if(headers[0] == "Message") {
            string name = headers[getHeaderIndex("Name")];
            string content = headers[getHeaderIndex("Content")];
            cout << name << ": " << content << "\n";
        }
        else if(headers[0]=="File"){
            mutex recv;
            recv.lock();
            pair<string, int> details = receivefile(sock,buffer);
            file_name = details.first;
            file_size = details.second;

            fstream file;
            file.open("./"+username+"/"+file_name,ios::binary);
            file.close();
            // write_to_file.open("./"+username+"/"+file_name,ios::binary);

            recv.unlock();
        }
        else if(headers[0] == "ConnectionCheck") {
            vector<string> response_headers(7);
            string name = headers[getHeaderIndex("Name")];
            string room = headers[getHeaderIndex("Room")];
            response_headers[0] = "Connect";
            response_headers[1] = "Client";
            response_headers[2] = name;
            response_headers[3] = room;
            response_headers[4] = "";
            response_headers[5] = "";
            response_headers[6] = "";
            string response = constructMessage(response_headers);
            send_mutex.lock();
            int send_status = (int)send(sock, response.c_str(), response.size(), 0);
            if( send_status <= 0) {
                perror("Error on sending. Terminating the connection\n");
                exit(1);
            }
            usleep(SLEEP_TIME);
            send_mutex.unlock();
        }
    }
}
void check_connection(int sock) {
    vector<string> response_headers(7);
    int sleeptime = 100000 * 20; //2.0 seconds
    while (1) {
        usleep(sleeptime);
        response_headers[0] = "ConnectionCheck";
        response_headers[1] = "Client";
        response_headers[2] = "";
        room_mutex.lock();
        response_headers[3] = to_string(room);
        room_mutex.unlock();
        response_headers[4] = "";
        response_headers[5] = "";
        response_headers[6] = "";
        string response = constructMessage(response_headers);
        send_mutex.lock();
        int send_status = (int)send(sock, response.c_str(), response.size(), 0);
        if( send_status <= 0) {
            perror("Error on sending. Terminating the connection\n");
            exit(1);
        }
        usleep(SLEEP_TIME);
        send_mutex.unlock();
    }
}
pair<string,int> receivefile(int sock,char buffer[]){

   // char buffer[BUF_SIZE ];
    char uname[50];
    strcpy(uname, username.c_str());
    mkdir(uname, 0777);

    string message(buffer);
    vector<string> headers = parse(message);
    string name = headers[getHeaderIndex("Name")];
    ///string content = headers[getHeaderIndex("Content")];
    string filedatawithname =  headers[getHeaderIndex("Content")];
    vector<string> filedata = parseByDelimeter(filedatawithname, "$", INT_MAX); //file1.png
    string filename = filedata[0];
    int filesize=stoi(filedata[1]);
    printf("received the filename and size !\n");
    cout<<"filename: "<<filename<<endl;
    return {filename,filesize};
}