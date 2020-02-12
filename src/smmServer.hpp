/*! @file
 * Defines the smmServer class and some auxillary classes to make working
 * with the underlying Mongoose HTTP server a bit cleaner.
 */

#ifndef SMM_SERVER_HPP
#define SMM_SERVER_HPP

#include <unordered_map>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include "mg/mongoose.h"

class httpMessage;

/*! @brief Helper typedef to make working with callback function pointers easier. */
//typedef void (*callback_t)(struct mg_connection*, struct http_message*, void*);
typedef void (*callback_t)(httpMessage, void*);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*! @brief Helper class that wraps underlying Mongoose server structures.
 *
 * This class is intended to make writing callbacks a little friendlier. However, the
 * underlying Mongoose structs are public if you'd prefer to use those instead.
 */
class httpMessage {
public:
  /*! @brief Pointer to the Mongoose mg_connection struct.
   *
   * This is included for advanced users who want to be able to more precisely control their
   * callback behavior. Most users should not need to access or modify it.
   */
  struct mg_connection* connection;

  /*! @brief Pointer to the Mongoose http_message struct.
   *
   * This is included for advanced users who want to be able to more precisely control their
   * callback behavior. Most users should not need to access or modify it.
   */  
  struct http_message* message;

  /*! @brief The Mongoose server's HTTP settings.
   *
   * This is included for advanced users who want to be able to more precisely control their
   * callback behavior. Most users should not need to access or modify it.
   */  
  struct mg_serve_http_opts httpOptions;
  
  /*! @brief Construct an httpMessage object.
   *
   * @param connection The Mongoose server connection that produced the request.
   * @param message The Mongoose server HTTP message.
   * @param httpOptions The Mongoose server's HTTP settings.
   */
  httpMessage(struct mg_connection* connection,
              struct http_message* message,
              struct mg_serve_http_opts httpOptions);

  /*! @brief Get an HTTP variable from the message.
   *
   * @param variableName String containing the name of the variable to extract.
   *
   * @returns A string containing the value of the requested variable if it exists,
   * or an empty string if it doesn't.
   */
  std::string getHttpVariable(std::string variableName);

  /*! @brief Respond with a simple <tt>200 OK</tt> message. */
  void replyHttpOk();

  /*! @brief Send an HTTP error response.
   *
   * @param code HTTP error code
   * @param reason Optional string containing the reason for the response.
   */
  void replyHttpError(int code, std::string reason="");

  /*! @brief Send some content via HTTP.
   *
   * @param mimeType The MIME type of the content being sent.
   * @param content A string containing the content to send.
   */
  void replyHttpContent(std::string mimeType, std::string content);
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*! @class smmServer
 * @brief The main server class.
 */
class smmServer {
private:
  static void handleEvent(struct mg_connection* connection, int event, void* event_data);

  bool beginServer();
  
  std::thread httpServerThread;
  std::atomic<bool> running;

  struct mg_connection* connection;
  struct mg_mgr eventManager;

  std::unordered_map<std::string, callback_t> postCallbackMap;
  std::unordered_map<std::string, callback_t>  getCallbackMap;

  std::mutex postCallbackMutex;
  std::mutex  getCallbackMutex;
  
public:
  /*! @brief The port to serve HTTP content over. */
  char* httpPort;

  /*! @brief The internal Mongoose server's HTTP settings.
   *
   * This is public only because of a need for static functions to be able to access
   * it via pointers, and so should not be modified by the user.
   */
  struct mg_serve_http_opts httpServerOptions;

  /*! @brief A pointer to user data that is passed to callback functions. 
   *
   * This server is multithreaded, so modifying the data pointed to by userData 
   * both within a callback and in another thread is not safe, and may cause a 
   * race condition. For this reason, it is recommended that you use a mutex
   * or other guard to control access to your userData object.
   */
  void* userData;

  /*! @brief smmServer constructor.
   *
   * @param port The port to serve HTTP content over.
   * @param path The path to the root directory of the web server.
   * @param userData A pointer to user-constructed data. This is passed to all callbacks.
   */
  smmServer(std::string port,
            std::string path,
            void* userData);

  /*! @brief smmServer destructor.
   *
   * The destructor safely shuts down the server and deallocates all resources.
   */
  ~smmServer();

  /*! @brief Start the server. */
  void launch();

  /*! @brief Stop the server */
  void shutdown();

  /*! @brief Returns the current state.
   *
   * @returns @c True if the server is running; @c False otherwise.
   */
  bool isRunning();

  /*! @brief Add a callback to a POST request.
   *
   * Adds a callback which can be invoked by sending a POST
   * request to @c /post with the HTTP variable @c callback set to @c name.
   *
   * @param name The string key to invoke the callback later.
   * @param callback Function pointer to the callback itself.
   */
  void addPostCallback(std::string name, callback_t callback);

  /*! @brief Retrieves a POST callback.
   *
   * @param name The string key of the callback to retrieve.
   *
   * @returns The callback stored with key @c name if it exists, and @c NULL otherwise.
   */
  callback_t retrievePostCallback(std::string name);

  /*! @brief Removes a POST callback.
   *
   * @param name The string key of the callback to remove.
   */
  void removePostCallback(std::string name);

  /*! @brief Add a callback to a GET request.
   *
   * Adds a callback which can be invoked by sending a GET
   * request to <tt>/get/[name]</tt>.
   *
   * @param name Final URI string to invoke the callback.
   * @param callback Function pointer to the callback itself.
   */
  void addGetCallback(std::string name, callback_t callback);

  /*! @brief Retrieves a GET callback.
   *
   * @param name The string key of the callback to retrieve.
   * 
   * @returns The callback stored with key @c name if it exists, and @c NULL otherwise.
   */  
  callback_t retrieveGetCallback(std::string name);

    /*! @brief Removes a GET callback.
   *
   * @param name The string key of the callback to remove.
   */
  void removeGetCallback(std::string name);
  
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#endif
