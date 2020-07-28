// g++ -o opb onepiece_bot.cpp -I ~/programming/cpp/libs/libtelegram/include -I ~/programming/cpp/libs/HTTPRequest/include -std=c++17 -pthread -lcrypto -lssl

#define TELEGRAM_NO_LISTENER_FCGI
#include <algorithm>
#include <chrono>
#include <fstream>
#include <mutex>
#include <set>
#include <string>
#include <thread>


#include "libtelegram/libtelegram.h"
#include "HTTPRequest.hpp"

// make user_ids and bots accessible for all threads
namespace op {
    // bot stuff
    telegram::sender *op_sender, *log_sender;
    telegram::listener::poll *op_listener;
    int_fast64_t admin_id{};

    // file access
    const std::string bot_path{"/home/llytaii/programming/cpp/onepiece_bot/"},
                        bot_data{bot_path + "bot_data.txt"},
                        fetcher_data{bot_path + "fetcher_data.txt"},
                        user_db{bot_path + "user_db.txt"};
    
    // data
    std::set<int_fast64_t> user_ids;
    bool lock{false};
    int nextChapter, nextEpisode;
}

// starting and stopping bot
bool init();
void run_bot();
void run_fetcher();
void stop();

// bot commands
namespace cmd{
    void start(int_fast64_t id);
    void end(int_fast64_t id);
}

namespace fetch{
    bool check_for(const std::string& search, const std::string& url);
    void notify(const std::string& notification);
}


int main()
{
    if(init())
    {
        std::thread bot_runner{run_bot};
        std::thread fetcher_runner{run_fetcher};
    }
    return 0;
}


bool init()
{
    if(op::op_sender || op::op_listener || op::log_sender)
        return false;
    
    /* init bots */
    std::string op_bot_token{}, log_bot_token{};
    std::fstream f{};

    f.open(op::bot_data, std::ios::in);
    if(!f.is_open())
        return false;
    f >> op_bot_token >> log_bot_token >> op::admin_id;
    f.close();
    
    op::op_sender = new telegram::sender{op_bot_token};
    op::op_listener = new telegram::listener::poll{*op::op_sender};
    op::log_sender = new telegram::sender{log_bot_token};
    op::op_listener->set_callback_message([](telegram::types::message const & message){
        if(!message.text)
            return;
        auto message_chat_id = message.chat.id;
        if(*message.text == "/start")
            cmd::start(message_chat_id);
        else if(*message.text == "/end")
            cmd::end(message_chat_id);
    });


    /* init user database */
    f.open(op::user_db, std::ios::in);
    if(!f.is_open())
    {
        op::log_sender->send_message(op::admin_id, "Failed to load user database!");
        return false;
    }

    int_fast64_t tmp{};
    while (f >> tmp)
    {
        if(op::user_ids.insert(tmp).second == false)
            op::log_sender->send_message(op::admin_id, "Found duplicate while loading user database: " + std::to_string(tmp));
    }
    f.close();

    /* read fetcher data */
    f.open(op::fetcher_data, std::ios::in);
    if(!f.is_open())
    {
        op::log_sender->send_message(op::admin_id, "Failed to load fetcher data!");
        return false;
    }
    f >> op::nextChapter >> op::nextEpisode;
    f.close();

    op::log_sender->send_message(op::admin_id, "Successfully initialized bot!");
    return true;
}

void run_bot()
{
    while (true)
    {
        //std::this_thread::sleep_for(std::chrono::minutes(2));
        op::op_listener->run();
        op::log_sender->send_message(op::admin_id, "Listener stopped running, trying to restart...");
    }
    

}

void run_fetcher()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        if(fetch::check_for("/kapitel/" + std::to_string(op::nextChapter), "http://onepiece-tube.com/kapitel-mangaliste"))
        {
            fetch::notify("https://onepiece-tube.com/kapitel/" + std::to_string(op::nextChapter) + "/1");
            std::fstream f;
            f.open("data.txt", std::ios::out);
            f << ++op::nextChapter << ' ' << op::nextEpisode;
            f.close();
        }
        
        if(fetch::check_for("/folge/" + std::to_string(op::nextEpisode), "http://onepiece-tube.com/episoden-streams"))
        {
            fetch::notify("https://onepiece-tube.com/folge/" + std::to_string(op::nextEpisode));
            std::fstream f;
            f.open("data.txt", std::ios::out);
            f << op::nextChapter << ' ' << ++op::nextEpisode;
            f.close();
        }
            
        
    }
    
}

void cmd::start(int_fast64_t id)
{
    while(op::lock);
    op::lock = true;
    if(op::user_ids.insert(id).second == true)
    {
        op::lock = false;
        std::fstream f{};
        f.open(op::user_db, std::ios::app);
        if(!f.is_open())
        {
            op::log_sender->send_message(op::admin_id, "Failed to add user to database: " + std::to_string(id));
        }
        f << id;
        f << ' ';
        f.close();
        op::op_sender->send_message(id, "Welcome to the OnePieceNotifyBot.\nSee /help for more info.");
    }
}

void cmd::end(int_fast64_t id)
{
    auto found{op::user_ids.find(id)};
    if(found == op::user_ids.end())
    {
        op::op_sender->send_message(id, "Failed to end subscription, your ID wasnt found.");
        return;
    }
    else
    {
        while(op::lock);
        op::lock = true;
        op::user_ids.erase(found);
        op::lock = false;
        op::op_sender->send_message(id, "Sorry, I wont bother you anymore.");

    }
    
        
}

bool fetch::check_for(const std::string& search, const std::string& url)
{
    http::Request request(url);
    const http::Response response = request.send("GET");
    std::string body = std::string(response.body.begin(), response.body.end());
    return (body.find(search) != std::string::npos);
}

void fetch::notify(const std::string& notification)
{   
    while(op::lock);
    op::lock = true;
    for(auto i : op::user_ids)
    {
        op::op_sender->send_message(i, notification);
    }
    op::lock = false;
}

