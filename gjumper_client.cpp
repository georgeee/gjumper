#include <unistd.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include "jsoncpp/json/json.h"
#include "communication.h"


using boost::asio::ip::tcp;

const char * const REQ_RESOLVE = "resolve";
const char * const REQ_RECACHE = "recache";
const char * const REQ_FULLRECACHE = "full_recache";

int print_usages(char * argv0){
    std::cout << argv0 << " pid prev_port type [type dependent args..]\n"
              << " type=" << REQ_RESOLVE << " args: filename line [index]\n"
              << " type=" << REQ_RECACHE << " args: filename\n"
              << " type=" << REQ_RESOLVE << " args: <nop>\n";
    return EXIT_FAILURE;
}

class server_instance_launched_exception : public std::exception {
    public:
        const int pid;
        const int port;
        server_instance_launched_exception(int pid, int port = 0) : pid(pid), port(port) {}
};

int launch_server(int pid, int *port){
    int fds[2];
    int ret = pipe(fds);
    if(ret < 0) return -1;
    ret = dup2(fds[1], STDOUT_FILENO);
    if(ret < 0) return -1;
    ret = fork();
    if(ret < 0) return -1;
    if(ret){
        FILE * in = fdopen(fds[0], "r");
        fscanf(in, "%d", port);
        fprintf(stderr, "Launched server on port %d\n", *port);
    }else{
        throw server_instance_launched_exception(pid);
    }
    return 0;
}

FILE* backup_stream(int fd, const char * mode){
    int fd2 = dup(fd);
    if(fd2 < 0) return NULL;
    return fdopen(fd2, mode);
}

class server{
    public:
        static const int DEFAULT_TIMEOUT = 5*1000;
        const int pid, port, pid_check_timeout;
        server(int pid, int port, int pid_check_timeout = DEFAULT_TIMEOUT) : pid(pid), port(port), pid_check_timeout(pid_check_timeout) {}

    private:
        gj::communication_relay relay;
        Json::FastWriter jsonWriter;
        boost::asio::io_service io_service;
    public:
        int launch(){
            try
            {
                auto pid_checker = [this] {
                    while(true){
                        std::this_thread::sleep_for(std::chrono::milliseconds(pid_check_timeout));
                        int ret = kill(pid, 0);
                        FILE * err = fopen("err.txt", "a");
                        fprintf(err, "Checking if pid=%d is still running: %s\n", pid, strerror(errno));
                        if(ret < 0 && errno == ESRCH){
                            std::exit(EXIT_SUCCESS);
                        }
                        fclose(err);
                    }
                };

                std::thread pid_checker_thread(pid_checker);

                tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
                std::cout << acceptor.local_endpoint().port() << std::endl << std::flush;

                for (;;)
                {
                    boost::asio::ip::tcp::iostream stream;
                    acceptor.accept(*stream.rdbuf());
                    boost::system::error_code ignored_error;
                    try{
                        Json::Value request;
                        stream >> request;
                        Json::Value response = relay.processRequest(request);
                        stream << jsonWriter.write(response) << (char) EOF;
                    }catch(std::exception & ex){
                    }
                }
            }
            catch (std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
            return EXIT_SUCCESS;
        }
};

constexpr const char * const RESP_PORT = "port";

void suppress_stderr(){
    int devNull = open("/dev/null", O_WRONLY);
    if(devNull < 0) return;
    dup2(devNull, STDERR_FILENO);
}

int main(int argc, char **argv)
{   
    FILE * stdout_backup = backup_stream(STDOUT_FILENO, "w");
    suppress_stderr();
    auto usages = std::bind(&print_usages, argv[0]);
    const int type_args_base = 4;
    if(argc < type_args_base)
        return usages();
    int pid;
    if(sscanf(argv[1], "%d", &pid) == 0)
        return usages();
    int prev_port;
    if(sscanf(argv[2], "%d", &prev_port) == 0)
        return usages();
    const char * req_type = argv[3];
    Json::Value request;
    if(strcmp(req_type, REQ_RESOLVE) == 0){
        if(argc < type_args_base + 2)
            return usages(); 
        const char * filename = argv[type_args_base];
        int line, index = -1;
        if(sscanf(argv[type_args_base + 1], "%d", &line) == 0)
            return usages();
        if(argc >= type_args_base + 3)
            if(sscanf(argv[type_args_base + 2], "%d", &index) == 0)
                return usages();
        request = gj::communication_relay::createResolveRequest(filename, line, index);
    }else if(strcmp(req_type, REQ_RECACHE) == 0){
        if(argc < type_args_base + 1)
            return usages(); 
        const char * filename = argv[type_args_base];
        request = gj::communication_relay::createRecacheRequest(filename);
    }else if(strcmp(req_type, REQ_FULLRECACHE) == 0){
        request = gj::communication_relay::createFullRecacheRequest();
    }else{
        return usages();
    }
    
    try{

        if(prev_port == 0){
            int status = launch_server(pid, &prev_port);
            if(status < 0){
                std::cerr << "Error occurred while launching server\n";
                return EXIT_FAILURE; 
            }
        }

        auto localhostAddress = boost::asio::ip::address::from_string("127.0.0.1");
        unique_ptr<tcp::iostream> stream(new tcp::iostream(tcp::endpoint(localhostAddress, prev_port)));
        if(stream->error()){
            int status = launch_server(pid, &prev_port);
            if(status < 0){
                std::cerr << "Error occurred while launching server\n" << std::flush;
                return EXIT_FAILURE; 
            }
            stream = unique_ptr<tcp::iostream>(new tcp::iostream(tcp::endpoint(localhostAddress, prev_port)));
            if(stream->error()){
                std::cerr << "Error connecting to server (port " << prev_port << ")\n" << std::flush;
                return EXIT_FAILURE; 
            }
        }
        stream->expires_from_now(boost::posix_time::seconds(10));
        Json::FastWriter jsonWriter;

        *stream << jsonWriter.write(request) << (char) EOF;
        Json::Value response;
        *stream >> response;
        response[RESP_PORT] = prev_port;
        fputs(jsonWriter.write(response).c_str(), stdout_backup);
        fclose(stdout_backup);
        return EXIT_SUCCESS;
    }catch(server_instance_launched_exception & ex){
        return server(ex.pid, ex.port).launch();
    }
}
