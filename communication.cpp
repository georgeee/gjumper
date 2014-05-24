#include <stdexcept>
#include <exception>
#include <cstring>

#include "communication.h"
#include "processor.h"
#include "hintdb_exporter.h"
#include "jsoncpp/json/json.h"

Json::Value gj::communication_relay::processRequest(const Json::Value & json){
    try{
        const char * type = json[REQ_TYPE].asCString();
        if(strcmp(type, REQ_TYPE_RESOLVE) == 0){
            return processResolveRequest(json);
        }else if(strcmp(type, REQ_TYPE_RECACHE) == 0){
            return processRecacheRequest(json);
        }else if(strcmp(type, REQ_TYPE_FULLRECACHE) == 0){
            return processFullRecacheRequest(json);
        }
    }catch(std::exception & ex){
    }
    return createResponseForInvalidRequest();
}


Json::Value gj::communication_relay::createResponseForError(){
    Json::Value json;
    json[RESP_STATUS] = RESP_STATUS_ERROR;
    return json;
}

Json::Value gj::communication_relay::createResponseForOk(){
    Json::Value json;
    json[RESP_STATUS] = RESP_STATUS_OK;
    return json;
}

Json::Value gj::communication_relay::createResponseForInvalidRequest(){
    Json::Value json;
    json[RESP_STATUS] = RESP_STATUS_INVALID_REQ;
    return json;
}

Json::Value gj::communication_relay::processFullRecacheRequest(const Json::Value & json){
    try{
        processor.full_recache();
    }catch(std::exception & ex){
        return createResponseForError();
    }
    return createResponseForOk();
}

Json::Value gj::communication_relay::processRecacheRequest(const Json::Value & json){
    const char * filename = json[REQ_FILENAME].asCString();
    try{
        processor.recache(filename);
    }catch(std::exception & ex){
        return createResponseForError();
    }
    return createResponseForOk();
}


Json::Value gj::communication_relay::processResolveRequest(const Json::Value & json){
    const char * filename = json[REQ_FILENAME].asCString();
    const Json::Value & posJson = json[REQ_POS];
    int line, index = -1;
    if(posJson.isIntegral()){
        line = posJson.asInt();
    }else if(posJson.isArray() && posJson.size() > 0){
        if(posJson.size() > 1){
            line = posJson[0].asInt();
            index = posJson[1].asInt();
        }else{
            line = posJson[0].asInt();
        }
    }else{
        throw std::runtime_error(std::string(REQ_POS) + " should be an array of either line and column or only line");
    }
    vector<shared_ptr<hint_t> > positions = index == -1
        ? processor.resolve_position(filename, line)
        : processor.resolve_position(filename, line, index);
    Json::Value response = createResponseForOk();
    response[RESP_DATA] = Json::Value();
    for(shared_ptr<hint_t> & pos : positions){
        response[RESP_DATA].append(gj::hintdb_json_exporter::getJSONValue(*pos));
    }
    return response;
}

Json::Value gj::communication_relay::createResolveRequest(const char * filename, int line, int index){
    Json::Value request;
    request[REQ_TYPE] = REQ_TYPE_RESOLVE;
    request[REQ_FILENAME] = filename;
    if(index == -1){
        request[REQ_POS] = line;
    }else{
        request[REQ_POS] = Json::Value();
        request[REQ_POS].append(line);
        request[REQ_POS].append(index);
    }
    return request;
}

Json::Value gj::communication_relay::createFullRecacheRequest(){
    Json::Value request;
    request[REQ_TYPE] = REQ_TYPE_FULLRECACHE;
    return request;
}

Json::Value gj::communication_relay::createRecacheRequest(const char * filename){
    Json::Value request;
    request[REQ_TYPE] = REQ_TYPE_RECACHE;
    request[REQ_FILENAME] = filename;
    return request;
}
