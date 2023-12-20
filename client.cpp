#include <iostream>
#include "SimplePocoHandler.h"
#include "amqp-cpp.h"

int menu() {
    std::cout << "What do you want:" << std::endl;
    std::cout << "1. Read new messenges" << std::endl;
    std::cout << "2. Write a messenge" << std::endl;
    std::cout << "3. Write a messenge with timer" << std::endl;
    std::string flag;
    // std::cin >> flag;
    getline(std::cin, flag, '\n');
    if (flag == "1") {
        return 1;
    } else if (flag == "2") {
        return 2;  
    } else if (flag == "3") {
        return 3;
    } else {
        std::cout << "Wrong walue" << std::endl; 
        menu();
    }
    return 0;
}


int main(void)
{

    SimplePocoHandler handler("localhost", 5672);
    AMQP::Connection connection(&handler, AMQP::Login("client", "client"), "/");
    AMQP::Channel server_channel(&connection);
    AMQP::Channel server_time_channel(&connection);
    AMQP::Channel client_channel(&connection);
    std::string login;
    std::cout << "Enter your login: ";
    std::cin >> login;
    std::string temp;
    getline(std::cin, temp, '\n');
    client_channel.declareQueue(login.c_str());


    std::string client_log = login + "log";
    AMQP::Channel client_log_channel(&connection);
    client_log_channel.declareQueue(client_log.c_str());


    server_channel.onReady([&]() {
        // std::string msg = login; // + " " + login + " " + "You are welcome!!!";
        server_channel.publish("", "server", login.c_str());
    });

    client_channel.consume(login.c_str(), AMQP::noack).onReceived(
            [&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
        {  
            if (message.bodySize() != 0) {
                std::string received_message{message.body()};
                received_message.resize(message.bodySize());
                std::cout << "Received " << received_message << std::endl;
            }
        }
    );

    client_log_channel.consume(client_log.c_str(), AMQP::noack).onReceived(
            [&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) 
        {
            if (message.bodySize() != 0) {
                int flag = menu();
                if (flag == 1) {
                    std::cout << "Unread messenges:" << std::endl;
                    server_channel.publish("", "server", login.c_str());
                } else if (flag == 2) {
                    std::cout << "Enter your message: ";
                    std::string client_message;
                    getline(std::cin, client_message, '\n');
                    std::string msg = login + " " + client_message;
                    server_channel.publish("", "server", msg.c_str());
                } else if (flag == 3) {
                    std::cout << "Set timer (seconds): ";
                    std::string time;
                    getline(std::cin, time, '\n');
                    std::cout << "\nEnter your message: ";
                    std::string client_message;
                    getline(std::cin, client_message, '\n');
                    std::string msg = time + "000 " + login + " " + client_message;
                    server_time_channel.publish("", "server_time", msg.c_str());
                } else {
                    std::cout << "Error";
                }
            }
        }
    );


    handler.loop();
    return 0;
}