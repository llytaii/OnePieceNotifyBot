// g++ -o fetcher fetcher.cpp -I ~/programming/cpp/libs/libtelegram/include -I ~/programming/cpp/libs/HTTPRequest/include -std=c++17 -pthread -lcrypto -lssl
#define TELEGRAM_NO_LISTENER_FCGI
#include <algorithm>
#include <fstream>
#include <thread>
#include <string>
#include <vector>


#include "libtelegram/libtelegram.h"
#include "HTTPRequest.hpp"

bool newChapter(int nextNumber){
    http::Request request("http://onepiece-tube.com/kapitel-mangaliste");
    const http::Response response = request.send("GET");
    std::string body = std::string(response.body.begin(), response.body.end());
    return (body.find("/kapitel/" + std::to_string(nextNumber)) != std::string::npos);
}
bool newEpisode(int nextNumber){
    http::Request request("http://onepiece-tube.com/episoden-streams");
    const http::Response response = request.send("GET");
    std::string body = std::string(response.body.begin(), response.body.end());
    return (body.find("/folge/" + std::to_string(nextNumber)) != std::string::npos);
}


int main(){
    int_fast64_t admin_id {};
    std::string const token("xxx");
    telegram::sender sender(token);

    

    
    std::fstream file{};
    int nextChapter{};
    int nextEpisode{};
    file.open("data.txt", std::ios::in);
    if(!file.is_open()){
        sender.send_message(admin_id, "Fetcher: Couldnt read Numbers!");
        return 1;
    }
    file >> nextChapter >> nextEpisode;
    file.close();

    std::vector<int_fast64_t> user_id_vector{};
    file.open("users.txt", std::ios::in);
    if(!file.is_open()){
        sender.send_message(admin_id, "Fetcher: Couldnt open user database!");
        return 1;
    }
    int_fast64_t tmp{};
    while (file.peek() != EOF)
    {
        file >> tmp;
        user_id_vector.push_back(tmp);
    }
    file.close();

    if(newChapter(nextChapter)){
        for(auto i : user_id_vector){
            sender.send_message(i, "https://onepiece-tube.com/kapitel/" + std::to_string(nextChapter) + "/1");
        }
        file.open("data.txt", std::ios::out);
        file << ++nextChapter << ' ' << nextEpisode;
        file.close();
    }
        if(newEpisode(nextEpisode)){
        for(auto i : user_id_vector){
            sender.send_message(i, "https://onepiece-tube.com/folge/" + std::to_string(nextEpisode));
        }
        file.open("data.txt", std::ios::out);
        file << nextChapter << ' ' << ++nextEpisode;
        file.close();
    }

    return 0;
}