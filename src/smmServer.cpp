#include <ctime>

#include "smmServer.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char dayName[][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char monthName[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static std::string getCurrentDateTime() {
  time_t t = time(NULL);
  struct tm* cTime = gmtime(&t);

  char result[32];
  snprintf(result, sizeof(result), "%s, %0*d %s %0*d %0*d:%0*d:%0*d GMT",
           dayName[cTime->tm_wday],
           2, cTime->tm_mday,
           monthName[cTime->tm_mon],
           4, cTime->tm_year + 1900,
           2, cTime->tm_hour,
           2, cTime->tm_min,
           2, cTime->tm_sec);

  return std::string(result);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

smmServer::smmServer(std::string port,
                     std::string path,
                     void* userData) :
  running(false),
  postCallbackMap(),
  getCallbackMap(),
  userData(userData),
  httpServerThread{} {
  // set up http port
  httpPort = (char*) malloc(256*sizeof(char));
  strcpy(httpPort, port.c_str());
  
  // set server options
  httpServerOptions.document_root = path.c_str();
  httpServerOptions.index_files = NULL;              
  httpServerOptions.per_directory_auth_file = NULL;  
  httpServerOptions.auth_domain = NULL;              
  httpServerOptions.global_auth_file = NULL;         
  httpServerOptions.enable_directory_listing = NULL; 
  httpServerOptions.ssi_pattern = NULL;              
  httpServerOptions.ip_acl = NULL;                   
  httpServerOptions.url_rewrites = NULL;             
  httpServerOptions.dav_document_root = NULL;        
  httpServerOptions.dav_auth_file = NULL;            
  httpServerOptions.hidden_file_pattern = NULL;      
  httpServerOptions.cgi_file_pattern = NULL;         
  httpServerOptions.cgi_interpreter = NULL;            
  httpServerOptions.custom_mime_types = NULL;        
  httpServerOptions.extra_headers = NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

smmServer::~smmServer() {
  shutdown();
  free(httpPort);
  mg_mgr_free(&eventManager);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::launch() {
  running = true;
  httpServerThread = std::thread{&smmServer::beginServer, this};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::shutdown() {
  running = false;
  if (httpServerThread.joinable()) {
    httpServerThread.join();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
bool smmServer::beginServer() {
  mg_mgr_init(&eventManager, this);
  connection = mg_bind(&eventManager, httpPort, handleEvent);
  if (connection == NULL) {
    std::cerr << "FATAL: mg_bind() failed! Is something else using the port?\n";
    running = false;
    return false;
  }
  
  mg_set_protocol_http_websocket(connection);

  while(running) {
    mg_mgr_poll(&eventManager,1000);
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool smmServer::isRunning() {
  return running;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::handleEvent(struct mg_connection* connection,
                            int event,
                            void* eventData) {
  smmServer* server = (smmServer*) connection->mgr->user_data;
  
  switch(event) {
  case MG_EV_HTTP_REQUEST:
    {
      // this is a POST callback request
      struct http_message* message = (struct http_message*) eventData;
      if (mg_vcmp(&message->uri, "/post") == 0) {
        char callbackKey[256];
        mg_get_http_var(&message->body, "callback", callbackKey, sizeof(callbackKey));
        callback_t callback = server->retrievePostCallback(callbackKey);
        if (callback != NULL) {
          callback(httpMessage(connection, message, server->httpServerOptions), server->userData);
        }
        else {
          mg_http_send_error(connection, 422, "Invalid callback key");
        }
      }
      // this is a GET callback request
      else if ( strstr(message->uri.p, "/get/") == message->uri.p ) {
        char callbackKey[256];
        int keyLen = message->uri.len - 5;
        strncpy(callbackKey, message->uri.p + 5, keyLen);
        callbackKey[keyLen] = 0; // correctly zero-terminate the string
        callback_t callback = server->retrieveGetCallback(callbackKey);
        if (callback != NULL) {
          callback(httpMessage(connection, message, server->httpServerOptions), server->userData);
        }
        else {
          mg_http_send_error(connection, 404, "Invalid callback key");
        }
      }
      // normal HTTP request
      else {
        mg_serve_http(connection, message, server->httpServerOptions);
      }
      break;
    }
  case MG_EV_SEND:
    {
      // do nothing
      break;
    }
  default:
    {
      // do nothing
      break;
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::addPostCallback(std::string name, callback_t callback) {
  postCallbackMutex.lock();
  postCallbackMap[name] = callback;
  postCallbackMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

callback_t smmServer::retrievePostCallback(std::string name) {
  callback_t cb;
  postCallbackMutex.lock();
  try {
    cb = postCallbackMap.at(name);
  }
  catch(std::out_of_range err) {
    std::cerr << "error: could not find POST callback with key '" << name <<"'" << std::endl;
    postCallbackMutex.unlock();
    return NULL;
  }
  postCallbackMutex.unlock();
  return cb;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::removePostCallback(std::string name) {
  postCallbackMutex.lock();
  postCallbackMap.erase(name);
  postCallbackMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::addGetCallback(std::string name, callback_t callback) {
  getCallbackMutex.lock();
  getCallbackMap[name] = callback;
  getCallbackMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

callback_t smmServer::retrieveGetCallback(std::string name) {
  callback_t cb;
  getCallbackMutex.lock();
  try {
    cb = getCallbackMap.at(name);
  }
  catch(std::out_of_range err) {
    std::cerr << "error: could not find GET callback with key '" << name <<"'" << std::endl;
    getCallbackMutex.unlock();
    return NULL;
  }
  getCallbackMutex.unlock();
  return cb;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void smmServer::removeGetCallback(std::string name) {
  getCallbackMutex.lock();
  getCallbackMap.erase(name);
  getCallbackMutex.unlock();
}
  
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

httpMessage::httpMessage(struct mg_connection* connection,
                         struct http_message* message,
                         struct mg_serve_http_opts httpOptions) :
  connection(connection),
  message(message),
  httpOptions(httpOptions) {}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::string httpMessage::getHttpVariable(std::string variableName) {
  char* decodedValue = (char*) malloc(256*sizeof(char));
  mg_get_http_var(&message->body, variableName.c_str(), decodedValue, sizeof(decodedValue));
  std::string result = std::string(decodedValue);
  free(decodedValue);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void httpMessage::replyHttpOk() {
  mg_send_response_line(connection, 200, httpOptions.extra_headers);
  mg_printf(connection,
            "Date: %s\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n",
            getCurrentDateTime().c_str());
}
  

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void httpMessage::replyHttpError(int code, std::string reason) {
  if (reason == "") {
    mg_http_send_error(connection, code, NULL);
  }
  else {
    mg_http_send_error(connection, code, reason.c_str());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void httpMessage::replyHttpContent(std::string mimeType, std::string content) {
  mg_send_response_line(connection, 200, httpOptions.extra_headers);
  mg_printf(connection,
            "Date: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n%s",
            getCurrentDateTime().c_str(),
            mimeType.c_str(),
            strlen(content.c_str()),
            content.c_str());
}
            

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
