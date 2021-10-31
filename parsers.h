#ifndef __PARSERS__
#define __PARSERS__

#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

pair<int, string> getContentUntilNewLine(string s, int pos);
int getHeaderIndex(string s);
vector<string> parse(string message);
vector<string> parseByDelimeter(string s, string del, int max);

vector<string> parse(string message) {
    /* Headers mapping
     0 - Type
     1 - Who
     2 - Name
     3 - Room
     4 - Receivers
     5 - SleepTime
     6 - Content
    */
    vector<string> headers = parseByDelimeter(message, "\n", 7);
    return headers;
}

int getHeaderIndex(string cur) {
    if(cur == "Type")
        return 0;
    else if(cur == "Who")
        return 1;
    else if(cur == "Name")
        return 2;
    else if(cur == "Room")
        return 3;
    else if(cur == "Receivers")
        return 4;
    else if(cur == "SleepTime")
        return 5;
    else if(cur == "Content")
        return 6;
    else
        return -1;
}

vector<string> parseByDelimeter(string s, string del, int max){
    vector<string> strs;
    int cur = 1;
    while(s.size()){
        if(cur == max)
            break;
        int i = (int)s.find(del);
        if(i != string::npos){
            strs.push_back(s.substr(0,i));
            s = s.substr(i+del.size());
            if(s.size()==0) {
                strs.push_back(s);
                cur++;
            }
        }else{
            strs.push_back(s);
            s = "";
        }
    }
    return strs;
}

string constructMessage(vector<string> headers) {
    string response = "";
    for(int i = 0; i < 7; i++) {
        if(i != 6)
            response += headers[i] + "\n";
        else
            response += headers[i];
    }
    return response;
}

string zipArray(vector<string> strs) {
    string response = "";
    for(auto i = 0; i < strs.size(); i++) {
        if(i != 0)
            response += "#";
        response += strs[i];
    }
    return response;
}
string zipArray(vector<string> strs, string del) {
    string response = "";
    for(auto i = 0; i < strs.size(); i++) {
        if(i != 0)
            response += del;
        response += strs[i];
    }
    return response;
}

#endif