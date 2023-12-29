#include <iostream>
#include <queue>
#include <chrono>
#include <thread>
#include <string>
#include "SimplePocoHandler.h"
#include "amqp-cpp.h"

using namespace std::chrono_literals;

class MessengeTimer {
public:
    MessengeTimer(std::chrono::_V2::system_clock::time_point s, 
                  std::chrono::_V2::system_clock::time_point e,
                  int64_t t, 
                  std::string msg) 
        : start(s), end(e), time(t), messenge(msg) {
    }
    MessengeTimer(MessengeTimer const & other)
        : start(other.start), end(other.end), time(other.time), messenge(other.messenge) {
    }
    std::chrono::_V2::system_clock::time_point start;
    std::chrono::_V2::system_clock::time_point end;
    int64_t time;
    std::string messenge;
};

struct Comp{
    bool operator()(MessengeTimer const & lhs, MessengeTimer const & rhs) {
        return (lhs.time > rhs.time); 
    }
};

void get_message_data(std::string& user_login, std::string& friend_login, std::string& msg, std::string const & data ) {
    user_login = "";
    friend_login = "";
    msg = "";
    int flag = 0;
    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == ' ' && flag != 2) {
            flag++;
            continue;
        }
        if (flag == 0) {
            user_login += data[i];
        }
        if (flag == 1) {
            friend_login += data[i];
        }
        if (flag == 2) {
            msg += data[i];
        }
    }
}

void get_message_data_with_timer(std::string& time, std::string& user_login, std::string& friend_login, std::string& msg, std::string const & data) {
    time = "";
    user_login = "";
    friend_login = "";
    msg = "";
    int flag = 0;

    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == ' ' && flag != 3) {
            flag++;
            continue;
        }
        if (flag == 0) {
            time += data[i];
        }
        if (flag == 1) {
            user_login += data[i];
        }
        if (flag == 2) {
            friend_login += data[i];
        }
        if (flag == 3) {
            msg += data[i];
        }
    }


}


int main(void)
{
    SimplePocoHandler handler("localhost", 5672);

    AMQP::Connection connection(&handler, AMQP::Login("admin", "admin"), "/");

    AMQP::Channel channel(&connection);
    // AMQP::Channel server_time_channel(&connection);
    // AMQP::Channel server_pq_channel(&connection);
    // server_channel.declareQueue("server");
    channel.declareQueue("server_time");
    channel.declareQueue("server_pq");
    // AMQP::Channel client_log_channel(&connection);
    // AMQP::Channel client_channel(&connection);




    // server_channel.consume("server", AMQP::noack).onReceived(
    //         [&](const AMQP::Message &message,
    //                    uint64_t deliveryTag,
    //                    bool redelivered)
    //     {  
    //         if (message.bodySize() != 0) {
    //             std::string received_message{message.body()};
    //             received_message.resize(message.bodySize());
    //             std::string user_login;
    //             std::string friend_login;
    //             std::string user_msg;
    //             get_message_data(user_login, friend_login, user_msg, received_message);
    //             std::cout <<" [x] Received: " << user_login << ' '<< friend_login << ' ' << user_msg << std::endl;
    //             std::string user_log = user_login + "log";
    //             client_log_channel.publish("", user_log.c_str(), "OK");
    //             std::string message_to_friend = user_login + ": " + user_msg;
    //             client_channel.publish("", friend_login.c_str(), message_to_friend.c_str());
    //         }
    //     }
    // );

    
    
    std::priority_queue<MessengeTimer, std::vector<MessengeTimer>, Comp> pq;

    channel.consume("server_time", AMQP::noack).onReceived(
            [&](const AMQP::Message &message,
                       uint64_t deliveryTag,
                       bool redelivered)
        {
            if (message.bodySize() != 0) {
                std::string received_message{message.body()};
                received_message.resize(message.bodySize());
                std::string time;
                std::string user_login;
                std::string friend_login;
                std::string user_msg;
                get_message_data_with_timer(time, user_login, friend_login, user_msg, received_message);
                std::cout <<" [x] Received: " << time << ' ' << user_login << ' '<< friend_login << ' ' << user_msg << std::endl;
                // std::string user_log = user_login + "log";
                // channel.publish("", user_log.c_str(), "OK");
                auto start = std::chrono::high_resolution_clock::now();
                auto end = std::chrono::high_resolution_clock::now();
                MessengeTimer new_timer{start, end, stoi(time), user_login + " " + friend_login + " " + user_msg};
                pq.push(new_timer);
                channel.publish("", "server_pq", "new item");
                // std::this_thread::sleep_for(2000ms);
                // std::cout << "Clock: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
                // std::string message_to_friend = user_login + ": " + user_msg;
                // client_channel.publish("", friend_login.c_str(), message_to_friend.c_str());
            }
        }
    );

    channel.consume("server_pq", AMQP::noack).onReceived(
            [&](const AMQP::Message &message,
                uint64_t deliveryTag,
                bool redelivered) 
        {
            if (message.bodySize() != 0 && !pq.empty()) {
                std::priority_queue<MessengeTimer, std::vector<MessengeTimer>, Comp> temp;
                // std::cout << "pq: "; // << pq.size() << std::endl;
                // while(!pq.empty()) {
                //     std::cout << pq.top().time << ' ';
                //     temp.push(pq.top());
                //     pq.pop();
                // }
                // std::cout << std::endl;
                // pq = temp;
                
                // while (!temp.empty()) {
                //     temp.pop();
                // } 
                
                while (!pq.empty()) {
                    MessengeTimer old_timer = pq.top();
                    auto end = std::chrono::high_resolution_clock::now();
                    int64_t new_time = old_timer.time - (std::chrono::duration_cast<std::chrono::milliseconds>(end - old_timer.end).count());
                    std::cout << "new time: " << new_time << std::endl;
                    MessengeTimer new_timer{old_timer.start,
                                            end,
                                            new_time,
                                            old_timer.messenge};
                    temp.push(new_timer);
                    // std::cout << "pq: " << pq.top().time << std::endl;
                    pq.pop();
                }
                pq = temp;
                // std::cout << "TOP: " << pq.top().time << std::endl;
                if (pq.top().time <= 0) {
                    // channel.publish("", )
                    std::string user = "";
                    std::string fr = "";
                    std::string msg = "";
                    get_message_data(user, fr, msg, pq.top().messenge.c_str());
                    channel.publish("", fr.c_str(), msg.c_str());




                    // server_channel.publish("", "server", pq.top().messenge.c_str());
                    std::cout << "Sent messenge with timer " << pq.top().messenge.c_str() << std::endl;
                    pq.pop();
                }
                std::this_thread::sleep_for(100ms);
                channel.publish("", "server_pq", "cycle");
            }
        }
    );


    std::cout << " [*] Waiting for messages. To exit press CTRL-C\n";
    handler.loop();
    return 0;
}