#include <iostream>
#include <memory>
#include <atomic>

#include "../hdr/command.h"
#include <FLogManager.h>
#include <config.h>
#include "../hdr/chat_client.h"
#include "../hdr/chat_server.h"

LEVEL FLogManager::mCurrentLevel = LEVEL::CRIT;
GRANULARITY FLogManager::mCurrentGranularity = GRANULARITY::FULL;

int main(int argc, char *argv[])
{
    std::cout << __FUNCTION__ << std::endl;

    //Input: FlashLogger <size_of_ring_buffer> <log_file_path> <log_file_name>
    FLogConfig config([](flashlogger_config_data &d, boost::program_options::options_description &desc){
        desc.add_options()
                ("Ahref.size_of_ring_buffer", boost::program_options::value<short>(&d.size_of_ring_buffer)->default_value(5), "size of buffer to log asyncoronously")
                ("Ahref.log_file_path", boost::program_options::value<std::string>(&d.log_file_path)->default_value("../"), "log file path")
                ("Ahref.log_file_name", boost::program_options::value<std::string>(&d.log_file_name)->default_value("flashlog.txt"), "log file name")
                ("Ahref.run_test", boost::program_options::value<short>(&d.run_test)->default_value(1), "choose to run test")
                ("Ahref.server_ip", boost::program_options::value<std::string>(&d.server_ip)->default_value("localhost"), "microservice server IP")
                ("Ahref.server_port", boost::program_options::value<std::string>(&d.server_port)->default_value("50051"), "microservice server port");
    });

    try {

        config.parse(argc, argv);
    }
    catch(std::exception const& e) {

        std::cout << e.what();
        return 0;
    }

    FLogManager::globalInstance(&config).SetCopyrightAndStartService(s_copyright);

    FLogManager::globalInstance().SetLogLevel("INFO");

    int input = 2;
    while(true){

        std::cout << "Login as Client[1] or Server[2] : ";
//        std::cin >> input;
        if (input == 1 || input == 2){

            std::unique_ptr<RkServer> server = std::make_unique<RkServer>("5000");
            server->execute();

            if (input == 1){

                std::unique_ptr<RkClient> client = std::make_unique<RkClient>(config.data().server_ip, config.data().server_port);
                client->execute();
            }
            break;
        }else{

            std::cout << "Invalid Input retry!!!" << std::endl;
            continue;
        }
    }

    bool exit;
    std::cin >> exit;
    return 0;
}
