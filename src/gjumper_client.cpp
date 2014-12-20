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
              << " type=" << REQ_FULLRECACHE << " args: <nop>\n";
    return EXIT_FAILURE;
}

class server_instance_launched_exception : public std::exception {
    public:
        const int pid;
        const FILE* outstream;
        server_instance_launched_exception(int pid, FILE* outstream) : pid(pid), outstream(outstream) {}
};

class server{
    public:
        static const int DEFAULT_TIMEOUT = 5*1000;
        const int pid, pid_check_timeout;
        unique_ptr<tcp::acceptor> acceptor;
        unique_ptr<boost::asio::io_service> io_service;
        server(int pid, unique_ptr<boost::asio::io_service> && io_service, unique_ptr<tcp::acceptor> && acceptor, int pid_check_timeout = DEFAULT_TIMEOUT) : pid(pid), pid_check_timeout(pid_check_timeout), io_service(std::move(io_service)), acceptor(std::move(acceptor)) {}

    private:
        gj::communication_relay relay;
        Json::FastWriter jsonWriter;
        std::unique_ptr<std::thread> pid_checker;
        std::unique_ptr<std::thread> server_instance;
    public:
        void launch(){
            if(pid){
                pid_checker = std::unique_ptr<std::thread>(new std::thread([this]{
                    try{
                        while(true){
                            std::this_thread::sleep_for(std::chrono::milliseconds(pid_check_timeout));
                            int ret = kill(pid, 0);
                            if(ret < 0 && errno == ESRCH){
                                std::exit(EXIT_SUCCESS);
                            }
                        }
                    }catch (std::exception const &exc){
                        std::cerr << "Pid checker thread: exception caught " << exc.what() << "\n" << std::flush;
                    }catch(...){
                        std::cerr << "Pid checker thread: unknown error caught" << std::endl << std::flush;
                    }
                }));
            }
            for (;;)
            {
                boost::asio::ip::tcp::iostream stream;
                acceptor->accept(*stream.rdbuf());
                boost::system::error_code ignored_error;
                try{
                    Json::Value request;
                    std::string reqString;
                    Json::Reader reader;
                    std::getline(stream, reqString, (char)0);
                    bool ok = reader.parse(reqString, request, false);
                    Json::Value response;
                    if(!ok){
                        response = relay.createResponseForInvalidRequest();
                    }else{
                        response = relay.processRequest(request);
                    }
                    stream << jsonWriter.write(response) << (char) 0 << std::flush;
                    std::cerr << "Response: " << jsonWriter.write(response) << (char) 0 << std::flush;
                }catch(std::exception & ex){
                    std::cerr << "Exception caught: " << ex.what() << std::flush;
                }
            }
        }
};

constexpr const char * const RESP_PORT = "port";

void suppress_stderr(){
    int errLog = open("/dev/null", O_WRONLY);
    if(errLog < 0) return;
    dup2(errLog, STDERR_FILENO);
}


int safe_launch_server(int pid, FILE * outstream, bool close_stream = true){
    unique_ptr<boost::asio::io_service> io_service; 
    unique_ptr<tcp::acceptor> acceptor;
    try{
        io_service = unique_ptr<boost::asio::io_service>(new boost::asio::io_service());
        acceptor = unique_ptr<tcp::acceptor>(new tcp::acceptor(*io_service.get(), tcp::endpoint(tcp::v4(), 0)));
        int port = acceptor->local_endpoint().port();
        fprintf(outstream, "%d\n", port);
    }catch(std::exception & ex){
        fprintf(outstream, "0\n");
        std::cerr << "exception caught: " << ex.what() << std::endl << std::flush;
        return -1;
    }
    if(close_stream) fclose(outstream);
    if(io_service && acceptor){
        try{
            server(pid, std::move(io_service), std::move(acceptor)).launch();
        }catch(std::exception & ex){
            std::cerr << "exception caught: " << ex.what() << std::endl << std::flush;
            return -1;
        }
    }
    return 0;
}

int fork_launch_server(int pid){
    int fds[2];
    int status = pipe(fds);
    if(status < 0) return 0;
    FILE * in = fdopen(fds[0], "r");
    FILE * out = fdopen(fds[1], "w");
    status = fork();
    if(status < 0) return 0;
    if(status){
        int port;
        fscanf(in, "%d", &port);
        return port;
    }else{
        exit(safe_launch_server(pid, out));
    }
}

int main(int argc, char **argv)
{   
    try{
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

        if(prev_port == -1){
            int port = safe_launch_server(pid, stdout, false);
            if(!port) return EXIT_FAILURE;
            std::cout << port << std::endl;
            return EXIT_SUCCESS;
        }
        suppress_stderr();


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

        if(prev_port == 0){
            prev_port = fork_launch_server(pid);
            if(!prev_port){
                std::cerr << "Error occurred while launching server\n" << std::flush;
                return EXIT_FAILURE; 
            }
        }

        auto localhostAddress = boost::asio::ip::address::from_string("127.0.0.1");
        unique_ptr<tcp::iostream> stream(new tcp::iostream(tcp::endpoint(localhostAddress, prev_port)));
        if(stream->error()){
            prev_port = fork_launch_server(pid);
            if(!prev_port){
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

        *stream << jsonWriter.write(request) << (char) 0 << std::flush;
        Json::Value response;
        std::string respString;
        Json::Reader reader;
        std::getline(*stream, respString, (char)0);
        if(respString.empty()) {
            response = communication_relay::createResponseForError();
        }else{
            bool ok = reader.parse(respString, response, false);
            if(!ok){
                throw runtime_error(std::string("errors while parsing text '") + respString + "' : " + reader.getFormattedErrorMessages());
            }
        }
        response[RESP_PORT] = prev_port;
        puts(jsonWriter.write(response).c_str());
        return EXIT_SUCCESS;
    }catch (std::exception const &exc){
        std::cerr << "Exception caught " << exc.what() << "\n";
        return 3;
    }catch(...){
        std::cerr << "Unknown error caught" << std::endl;
        return 3;
    }
}
