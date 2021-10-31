#ifndef __STRUCTS__
#define __STRUCTS__

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "parsers.h"
#include <stdexcept>
using namespace std;
#define SLEEP_TIME 30000

struct User {
    string name;
    int sock;
    bool operator==(const User &rhs) {
        return name == rhs.name && sock == rhs.sock;
    }
    User(string name_, int sock_) {
        name = name_;
        sock = sock_;
    }
    User(const User& other) {
        name = other.name;
        sock = other.sock;
    }
};

class Room {
public:
    int num;
    vector<User> users;
    Room(int roomNum){
        num = roomNum;
    }
    void deleteUser(int sock) {
        for(auto it = users.begin(); it != users.end(); it++) {
            if(it->sock == sock){
                users.erase(it);
                break;
            }
        }
    }
    void addUser(User newUser) {
        users.push_back(newUser);
    }
    int getRoomNum(){
        return num;
    }
    vector<User> getUserList() {
        return users;
    }
    int getSock(string name) {
        for(auto it = users.begin(); it != users.end(); it++) {
            if(it->name == name){
                return it->sock;
            }
        }
        return -1;
    }
    string getUser(int sock) {
        for(auto it = users.begin(); it != users.end(); it++) {
            if(it->sock == sock){
                return it->name;
            }
        }
        throw runtime_error("Can't get user");
    }
    
};

class Database {
    vector<Room> database;
    mutex access_mutex;
public:
    Database(){}
    int getSockForFile(int room, string client_name){
        return database[getRoomIndex(room)].getSock(client_name);
    }
    void deleteUser(int room, int sock) {
        int index = getRoomIndex(room);
        if(index == -1)
            return;
        database[index].deleteUser(sock); 
    }
    int getRoomIndex(int roomNum) {
        for(auto i = 0; i < database.size(); i++ ) {
            if(database[i].getRoomNum() == roomNum) {
                return i;
            }
        }
        return -1;
    }
    int addUser(int roomNum, User user) {
        access_mutex.lock();
        int index = getRoomIndex(roomNum);
        if(index == -1) {
            // If room is not created yet
            database.push_back(Room(roomNum));
            index = (int)database.size() - 1;
        }
        // Start to solve name duplication
        bool duplicate;
        int count = 1;
        while(1) {
            duplicate = false;

            string curname = user.name;
            if(count != 1) {
                curname = curname + "-" + to_string(count);
            }
            for(auto j = 0; j < database.size(); j++) {
                for(auto i = 0; i < database[j].users.size(); i++) {
                    if(database[j].users[i].name == curname) {
                        count++;
                        duplicate = true;
                        break;
                    }
                }
                if(duplicate == true)
                    break;
            }
            if(!duplicate) {
                user.name = curname;
                break;
            }
        }
        //End of solving name duplication
        database[index].addUser(user);
        access_mutex.unlock();
        return count;
    }
    vector<User> getUserList(int room) {
        access_mutex.lock();
        auto users = database[getRoomIndex(room)].getUserList();
        access_mutex.unlock();
        return users;
    }
    vector<Room> getRoomList(){
         access_mutex.lock();
        auto listofrooms = database;
        access_mutex.unlock();
        return listofrooms;
    }
    void sendWithSleep(int sock, string data, int delay, mutex *m_ref) {
        usleep(delay);
        m_ref->lock();
        send(sock, data.c_str(), data.size(), 0);
        usleep(SLEEP_TIME);
        m_ref->unlock();
    }
    
    void sendMessageHelper(int from, vector<string> to, string message, int room, vector<int> times, mutex *m_ref) {
        vector<string> response_headers(7);
        response_headers[0] = "Message";
        response_headers[1] = "Server";
        response_headers[2] = database[getRoomIndex(room)].getUser(from);
        response_headers[3] = to_string(room);
        response_headers[4] = "";
        response_headers[5] = "";
        response_headers[6] = message;
        //check if the users are in the database
        vector<string> misnames;
        for(auto i = 0; i < to.size(); i++) {
            if(database[getRoomIndex(room)].getSock(to[i]) == -1)
                misnames.push_back(to[i]);
        }
        //if misnames is empty, there is no error
        if(!misnames.empty()) {
            response_headers[0] = "Error";
            response_headers[6] = "There is no such user: " + zipArray(misnames, ", ");
            string response = constructMessage(response_headers);
            m_ref->lock();
            send(from, response.c_str(), response.size(), 0);
            usleep(SLEEP_TIME);
            m_ref->unlock();
            return;
        }
        //No error
        string response = constructMessage(response_headers);
        for(auto i = 0; i < to.size(); i++) {
            int sock = database[getRoomIndex(room)].getSock(to[i]);
            if(times[i] != 0) {
                int microsec = 1000000 * times[i];
                thread *task = new thread(&Database::sendWithSleep, this, sock,  response, microsec, m_ref);
            }
            else {
                m_ref->lock();
                send(sock, response.c_str(), response.size(), 0);
                usleep(SLEEP_TIME);
                m_ref->unlock();
            }
        }
    }
    
    void changeRoom(int oldRoom, int newRoom, int sock){
        access_mutex.lock();
        vector<string> response_headers(7);
        string name = database[getRoomIndex(oldRoom)].getUser(sock);
        response_headers[0] = "ChangeRoom";
        response_headers[1] = "Server";
        response_headers[2] = name;
        response_headers[3] = to_string(oldRoom);
        response_headers[4] = "Left";
        response_headers[5] = "";
        response_headers[6] = to_string(newRoom);
        string response = constructMessage(response_headers);
        auto users = database[getRoomIndex(oldRoom)].getUserList();
        for(auto i = 0; i < users.size(); i++) {
            if(users[i].sock != sock) {
                send(users[i].sock, response.c_str(), response.size(), 0);
            }
        }
        database[getRoomIndex(oldRoom)].deleteUser(sock);
        response_headers[4] = "Join";
        response = constructMessage(response_headers);
        users.clear();
        if(getRoomIndex(newRoom) == -1) {
            //Add a room if it does not exist
            database.push_back(Room(newRoom));
        }
        users = database[getRoomIndex(newRoom)].getUserList();
        for(auto i = 0; i < users.size(); i++) {
            send(users[i].sock, response.c_str(), response.size(), 0);
        }
        User newUser = User(name, sock);
        database[getRoomIndex(newRoom)].addUser(newUser);
        response_headers[4] = "Both";
        response = constructMessage(response_headers);
        cout << response << "\n";
        send(sock, response.c_str(), response.size(), 0);
        access_mutex.unlock();
    }
        
    void sendMessage(int from, vector<string> to, string message, int room, vector<int> times, mutex *m_ref) {
        access_mutex.lock();
        sendMessageHelper(from, to, message, room, times, m_ref);
        access_mutex.unlock();
    }
    
    void sendMessageToAll(int sock, int room, string type, string message) {
        access_mutex.lock();
        vector<string> response_headers(7);
        string name = database[getRoomIndex(room)].getUser(sock);
        response_headers[0] = "Error"; //Error field is used as notification field
        response_headers[1] = "Server";
        if(type == "Leave")
            response_headers[6] = name + " is disconnected from room #" + to_string(room);
        else if(type == "Join")
            response_headers[6] = name + " joined the room #" + to_string(room);
        else if(type == "Message") {
            response_headers[0] = "Message";
            response_headers[getHeaderIndex("Name")] = name;
            response_headers[getHeaderIndex("Room")] = to_string(room);
            response_headers[getHeaderIndex("Content")] = message;
        }
        string response = constructMessage(response_headers);
        int index = getRoomIndex(room);
        if(index != -1) {
            auto users = database[getRoomIndex(room)].getUserList();
            for(auto i = 0; i < users.size(); i++) {
                if(sock != users[i].sock)
                    send(users[i].sock, response.c_str(), response.size(), 0);
            }
        }
        if(type == "Leave") {
            response_headers[0] = "Disconnect";
            response_headers[getHeaderIndex("Room")] = to_string(room);
            response_headers[getHeaderIndex("Name")] = name;
            response = constructMessage(response_headers);
            send(sock, response.c_str(), response.size(), 0);
        }
        access_mutex.unlock();
    }
    void sendFile(int from, vector<string> to, string message, int room, vector<int> times, mutex *m_ref) {
        access_mutex.lock();
        sendFileHelper(from, to, message, room, times, m_ref);
        access_mutex.unlock();
    }
    void sendFileHelper(int from, vector<string> to, string content, int room, vector<int> times, mutex *m_ref) {
        vector<string> response_headers(7);
        response_headers[0] = "File";
        response_headers[1] = "Server";
        response_headers[2] = database[getRoomIndex(room)].getUser(from);
        response_headers[3] = to_string(room);
        response_headers[4] = "";
        response_headers[5] = "";
        response_headers[6] = content;
        //check if the users are in the database
        vector<string> misnames;
        for(auto i = 0; i < to.size(); i++) {
            if(database[getRoomIndex(room)].getSock(to[i]) == -1)
                misnames.push_back(to[i]);
        }
        //if misnames is empty, there is no error
        if(!misnames.empty()) {
            response_headers[0] = "Error";
            response_headers[6] = "There is no such user: " + zipArray(misnames, ", ");
            string response = constructMessage(response_headers);
            m_ref->lock();
            send(from, response.c_str(), response.size(), 0);
            usleep(SLEEP_TIME);
            m_ref->unlock();
            return;
        }
        //No error
        string response = constructMessage(response_headers);
        for(auto i = 0; i < to.size(); i++) {
            int sock = database[getRoomIndex(room)].getSock(to[i]);
            if(times[i] != 0) {
                int microsec = 1000000 * times[i];
                thread *task = new thread(&Database::sendWithSleep, this, sock,  response, microsec, m_ref);
            }
            else {
                m_ref->lock();
                send(sock, response.c_str(), response.size(), 0);
                usleep(SLEEP_TIME);
                m_ref->unlock();
            }
        }
    }
    void sendFileToAll(int sock, int room, string type, string content) {
        access_mutex.lock();
        vector<string> response_headers(7);
        string name = database[getRoomIndex(room)].getUser(sock);
        response_headers[0] = "Error"; //Error field is used as notification field
        response_headers[1] = "Server";
        if(type == "File") {
            response_headers[0] = "File";
            response_headers[getHeaderIndex("Name")] = name;
            response_headers[getHeaderIndex("Room")] = to_string(room);
            response_headers[getHeaderIndex("Content")] = content;
        }
        string response = constructMessage(response_headers);

        cout<<"ERR: Message sent by server : "<<response<<endl;
        
        char buffer[512];
        memset(buffer,'\0',512);
        for(int i = 0 ; i < response.size() ; ++i)
            buffer[i] = response[i];
        
        int index = getRoomIndex(room);
        if(index != -1) {
            auto users = database[getRoomIndex(room)].getUserList();
            for(auto i = 0; i < users.size(); i++) {
                if(sock != users[i].sock)
                    send(users[i].sock, buffer, sizeof(buffer), 0);
            }
        }
        access_mutex.unlock();
    }
    void sendLeaveMessage(int sock, int room) {
        sendMessageToAll(sock, room, "Leave", "");
    }
    void sendJoinMessage(int sock, int room) {
        sendMessageToAll(sock, room, "Join", "");
    }

};
#endif