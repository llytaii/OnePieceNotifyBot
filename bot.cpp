// g++ -o t main.cpp -I ~/programming/cpp/onepiece_bot/include/ -std=c++17 -pthread -lcrypto -lssl

#define TELEGRAM_NO_LISTENER_FCGI
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>


#include "libtelegram/libtelegram.h"


auto main()->int {
  int_fast64_t admin_id {259622761};
  std::string users_txt {"users.txt"};

  std::fstream users{};
  std::vector<int_fast64_t> chat_ids{};

  users.open(users_txt, std::ios::in);
  if(users.is_open()){
    int_fast64_t tmp;
    while(users.peek() != EOF){
      users >> tmp;
      chat_ids.push_back(tmp);
    }
    users.close();
  }

  

  std::string const token("942517226:AAFCBFzO06ItnNDntK3KFGsi9HfDqp8t3Yk");

  telegram::sender sender(token);                                               
  telegram::listener::poll listener(sender); 

  listener.set_callback_message([&](telegram::types::message const &message){   

    if(!message.text){
      return;
      
    } else {
      auto message_chat_id = message.chat.id;

      if(*message.text == "/start"){

        if(std::find(chat_ids.begin(), chat_ids.end(), message_chat_id) == chat_ids.end()){
                  chat_ids.push_back(message_chat_id);
                  users.open(users_txt, std::ios::app);

                  if(!users.is_open()){
                    sender.send_message(message_chat_id, "Failed saving ID, please try again!");
                    return;
                  }
                  users << message_chat_id;
                  users << ' ';
                  users.close();
                  sender.send_message(message_chat_id, "Welcome!");
        }
      } else if(*message.text == "/end"){
        auto found{std::find(chat_ids.begin(), chat_ids.end(), message_chat_id)};
        if(found == chat_ids.end()){
          sender.send_message(message_chat_id, "Failed to end subscription, ID not found.");
        } else {
          chat_ids.erase(found);
          users.open(users_txt, std::ios::out);
          if(!users.is_open()){
            sender.send_message(message_chat_id, "Failed to delete id, couldnt open file.");
            return;
          }
          for(auto i : chat_ids){
            users << i;
            users << ' ';
          }
          users.close();
          sender.send_message(message_chat_id, "Successfully ended your subscription.");
        }
      }
    } 

  });

  listener.run();              

  return EXIT_SUCCESS;
}
