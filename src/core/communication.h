#ifndef GJ_COMMUNICATION_H
#define GJ_COMMUNICATION_H

#include "processor.h"
#include "jsoncpp/json/json.h"

namespace gj{
    class communication_relay{
        static constexpr const char * const REQ_TYPE = "type";
        static constexpr const char * const REQ_TYPE_RESOLVE = "resolve";
        static constexpr const char * const REQ_TYPE_RECACHE = "recache";
        static constexpr const char * const REQ_TYPE_FULLRECACHE = "full_recache";
        static constexpr const char * const REQ_POS = "pos";
        static constexpr const char * const REQ_FILENAME = "filename";
        static constexpr const char * const RESP_STATUS = "status";
        static constexpr const char * const RESP_DATA = "data";
        static constexpr const int RESP_STATUS_OK = 1;
        static constexpr const int RESP_STATUS_INVALID_REQ = 2;
        static constexpr const int RESP_STATUS_ERROR = 3;
        gj::processor processor;

        
        Json::Value processResolveRequest(const Json::Value & json);
        Json::Value processRecacheRequest(const Json::Value & json);
        Json::Value processFullRecacheRequest(const Json::Value & json);
        static Json::Value createResponseForInvalidRequest();
        static Json::Value createResponseForOk();
        static Json::Value createResponseForError();


        public:
        static Json::Value createResolveRequest(const char * filename, int line, int index = -1);
        static Json::Value createRecacheRequest(const char * filename);
        static Json::Value createFullRecacheRequest();
        Json::Value processRequest(const Json::Value & json);
    };

};

#endif
