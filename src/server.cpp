#include "server.h"

PsychicHttpServer server;

// Variable to hold the last modification datetime
char last_modified[50];

QueueHandle_t wsRequests;

int authenticatedConnections[YB_CLIENT_LIMIT];

String server_cert;
String server_key;

void server_setup()
{
  //prepare our message queue
  wsRequests = xQueueCreate(YB_RECEIVE_BUFFER_COUNT, sizeof(WebsocketRequest));
  if (wsRequests == 0)
    Serial.printf("Failed to create queue= %p\n", wsRequests);

  server.config.max_open_sockets = YB_CLIENT_LIMIT;
  // Populate the last modification date based on build datetime
  sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);

  //do we want https?
  if (preferences.isKey("appEnableSSL"))
    app_enable_ssl = preferences.getBool("appEnableSSL");
  else
    app_enable_ssl = false;

  if (app_enable_ssl)
    Serial.println("SSL enabled");
  else
    Serial.println("SSL disabled");

  //look up our keys?
  if (app_enable_ssl)
  {
    File fp = LittleFS.open("/server.crt");
    if (fp)
      server_cert = fp.readString();
    else
    {
      Serial.println("LittleFS /server.pem not found, SSL not available");
      app_enable_ssl = false;
    }
    fp.close();

    File fp2 = LittleFS.open("/server.key");
    if (fp2)
      server_key = fp2.readString();
    else
    {
      Serial.println("LittleFS /server.key not found, SSL not available");
      app_enable_ssl = false;
    }
    fp2.close();
  }

  //do we want secure or not?
  if (app_enable_ssl)
    server.listen(443, server_cert.c_str(), server_key.c_str());
  else
    server.listen(80);

  server.on("/", HTTP_GET, [](PsychicHttpServerRequest *request) {

    //Check if the client already has the same version and respond with a 304 (Not modified)
    if (request->header("If-Modified-Since").indexOf(last_modified) > 0)
    {
      TRACE();
      return request->reply(304);
    }
    //What about our ETag?
    else if (request->header("If-None-Match").equals(YB_FIRMWARE_VERSION))
    {
      TRACE();
      return request->reply(304);
    }
    else
    {
        PsychicHttpServerResponse response(request);
        response.setCode(200);
        response.setContentType("text/html");

        // Tell the browswer the contemnt is Gzipped
        response.addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response.addHeader("Last-Modified", last_modified);
        response.addHeader("ETag", YB_FIRMWARE_VERSION);
        //response.addHeader("Cache-Control", "max-age=604800"); // cache for 1 week

        //add our actual content
        response.setContent(index_html_gz, index_html_gz_len);

        return response.send();
    }
  });

  server.on("/logo-navico.png", HTTP_GET, [](PsychicHttpServerRequest *request)
  {
    PsychicHttpServerResponse response(request);
    response.setCode(200);
    response.setContentType("image/png");

    // Tell the browswer the contemnt is Gzipped
    response.addHeader("Content-Encoding", "gzip");

    // And set the last-modified datetime so we can check if we need to send it again next time or not
    response.addHeader("Last-Modified", last_modified);
    //response.addHeader("ETag", YB_FIRMWARE_VERSION);

    //add our actual content
    response.setContent(logo_navico_gz, logo_navico_gz_len);

    return response.send();
  });

  // Test the stream response class
  server.websocket("/ws")->
    onFrame([](PsychicHttpWebSocketRequest *request, httpd_ws_frame *frame) {
      handleWebSocketMessage(request, frame->payload, frame->len);
      return ESP_OK;
    })->
    onConnect([](PsychicHttpWebSocketRequest *request) {
      Serial.printf("[socket] connected (#%u)\n", request->connection->id());
      websocketClientCount++;
      return ESP_OK;
    })->
    onClose([](PsychicHttpServer *server, int sockfd) {
      Serial.printf("[socket] closed (#%u)\n", sockfd);
      websocketClientCount--;
      return ESP_OK;
    });

    server.onOpen([](httpd_handle_t hd, int sockfd) {
      httpClientCount++;
      return ESP_OK;
    });

    server.onClose([](httpd_handle_t hd, int sockfd) {
      httpClientCount--;
      return ESP_OK;
    });


  //our main api connection
  server.on("/api/endpoint", HTTP_POST, [](PsychicHttpServerRequest *request)
  {
    StaticJsonDocument<1024> json;

    String body = request->body();
    DeserializationError err = deserializeJson(json, body);
    return handleWebServerRequest(json, request);
  });

  //send config json
  server.on("/api/config", HTTP_GET, [](PsychicHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_config";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //send stats json
  server.on("/api/stats", HTTP_GET, [](PsychicHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_stats";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //send update json
  server.on("/api/update", HTTP_GET, [](PsychicHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_update";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //downloadable coredump file
  server.on("/coredump.txt", HTTP_GET, [](PsychicHttpServerRequest *request)
  {
    //delete the coredump here, but not from littlefs
		deleteCoreDump();

    //dont bug the client anymore
    has_coredump = false;

    PsychicHttpServerResponse response(request);
    response.setContentType("text/plain");

    if (LittleFS.exists("/coredump.txt"))
    {
      response.setCode(200);

      //this feels a little bit hacky...
      File fp = LittleFS.open("/coredump.txt");
      String data = fp.readString();
      response.setContent(data.c_str());
    }
    else
    {
      response.setCode(404);
      response.setContent("Coredump not found.");
    }

    return response.send();
  });
}

void server_loop()
{
  //process our websockets outside the callback.
  WebsocketRequest request;
  if (xQueueReceive(wsRequests, &request, 0) == pdTRUE)
  {
    handleWebsocketMessageLoop(&request);

    //make sure to release our memory!
    free(request.buffer);
  }
}

void sendToAllWebsockets(const char * jsonString)
{
  //send the message to all authenticated clients.
  if (require_login)
  {
    for (byte i=0; i<YB_CLIENT_LIMIT; i++)
      if (authenticatedConnections[i])
      {
        PsychicHttpWebSocketConnection connection(server.server, authenticatedConnections[i]);
        connection.queueMessage(jsonString);
      }
  }
  //nope, just sent it to all.
  else
    server.sendAll(jsonString);
}

bool logClientIn(PsychicHttpWebSocketConnection *connection)
{
  //did we not find a spot?
  if (!addClientToAuthList(connection))
  {
    Serial.println("Error: could not add to auth list.");

    //i'm pretty sure this closes our connection
    close(connection->id());

    return false;
  }

  return true;
}

esp_err_t handleWebServerRequest(JsonVariant input, PsychicHttpServerRequest *request)
{
  esp_err_t err = ESP_OK;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  if (request->hasParam("user"))
    input["user"] = request->getParam("user");
  if (request->hasParam("pass"))
    input["pass"] = request->getParam("pass");

  if (app_enable_api)
    handleReceivedJSON(input, output, YBP_MODE_HTTP);
  else
    generateErrorJSON(output, "Web API is disabled.");      

  //we can have empty messages
  if (output.size())
  {
    //allocate memory for this output
    size_t jsonSize = measureJson(output);
    char * jsonBuffer = (char *)malloc(jsonSize+1);
    jsonBuffer[jsonSize] = '\0'; // null terminate

    //did we get anything?
    if (jsonBuffer != NULL)
    {
      PsychicHttpServerResponse response(request);
      response.setContentType("application/json");
      serializeJson(output.as<JsonObject>(), jsonBuffer, jsonSize+1);
      response.setContent(jsonBuffer);
      err = response.send();
    }
    //send overloaded response
    else
      err = request->reply(503, "application/json", "{}");

    //no leaks!
    free(jsonBuffer);
  }
  //give them valid json at least
  else
    err = request->reply(200, "application/json", "{}");

  return err;
}

void handleWebSocketMessage(PsychicHttpWebSocketRequest *request, uint8_t *data, size_t len)
{
  //build our websocket request - copy the existing one
  //we are allocating memory here, and the worker will free it
  WebsocketRequest wr;
  wr.client_id = request->connection->id();
  wr.len = len+1;
  wr.buffer = (char *)malloc(len+1);

  //did we flame out?
  if (wr.buffer == NULL)
  {
    Serial.printf("Queue message: unable to allocate %d bytes\n", len+1);
    return;    
  }

  //okay, copy it over
  memcpy(wr.buffer, data, len+1); 

  //throw it in our queue
  if (xQueueSend(wsRequests, &wr, 1) != pdTRUE)
  {
    Serial.println("[socket] work queue full");

    //free the memory... no worker to do it for us.
    free(wr.buffer);
  }

  //send a throttle message if we're full
  if (!uxQueueSpacesAvailable(wsRequests))
  {
    StaticJsonDocument<128> output;

    //dynamically allocate our buffer
    size_t jsonSize = measureJson(output);
    char * jsonBuffer = (char *)malloc(jsonSize+1);
    jsonBuffer[jsonSize] = '\0'; // null terminate

    //did we get anything?
    if (jsonBuffer != NULL)
    {
      generateErrorJSON(output, "Queue Full");
      serializeJson(output, jsonBuffer, jsonSize+1);
      request->reply(jsonBuffer);
    }
    free(jsonBuffer);
  }
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);
  DynamicJsonDocument input(1024);

  PsychicHttpWebSocketConnection connection(server.server, request->client_id);

  //was there a problem, officer?
  DeserializationError err = deserializeJson(input, request->buffer);
  if (err)
  {
    char error[64];
    sprintf(error, "deserializeJson() failed with code %s", err.c_str());
    generateErrorJSON(output, error);
  }
  else
    handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, &connection);

  //empty messages are valid, so don't send a response
  if (output.size())
  {
    //allocate memory for this output
    size_t jsonSize = measureJson(output);
    char * jsonBuffer = (char *)malloc(jsonSize+1);
    jsonBuffer[jsonSize] = '\0'; // null terminate

    //did we get anything?
    if (jsonBuffer != NULL)
    {
      serializeJson(output, jsonBuffer, jsonSize+1);
      connection.queueMessage(jsonBuffer);

      //keep track!
      sentMessages++;
      totalSentMessages++;
    }

    free(jsonBuffer);    
  }
}

bool isLoggedIn(JsonVariantConst input, byte mode, PsychicHttpWebSocketConnection *connection)
{
  //also only if enabled
  if (!require_login)
    return true;

  //login only required for websockets.
  if (mode == YBP_MODE_WEBSOCKET)
    return isWebsocketClientLoggedIn(input, connection);
  else if (mode == YBP_MODE_HTTP)
    return isApiClientLoggedIn(input);
  else if (mode == YBP_MODE_SERIAL)
    return isSerialClientLoggedIn(input);
  else
    return false;
}

bool isWebsocketClientLoggedIn(JsonVariantConst doc, PsychicHttpWebSocketConnection *connection)
{
  //are they in our auth array?
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    if (authenticatedConnections[i] == connection->id())
      return true;

  //okay check for passed-in credentials
  return isApiClientLoggedIn(doc);
}

bool isApiClientLoggedIn(JsonVariantConst doc)
{
  if (!doc.containsKey("user"))
    return false;
  if (!doc.containsKey("pass"))
    return false;

  //init
  char myuser[YB_USERNAME_LENGTH];
  char mypass[YB_PASSWORD_LENGTH];
  strlcpy(myuser, doc["user"] | "", sizeof(myuser));
  strlcpy(mypass, doc["pass"] | "", sizeof(myuser));

  //morpheus... i'm in.
  if (!strcmp(app_user, myuser) && !strcmp(app_pass, mypass))
    return true;

  //default to fail then.
  return false;  
}

bool isSerialClientLoggedIn(JsonVariantConst doc)
{
  if (is_serial_authenticated)
    return true;
  else
    return isApiClientLoggedIn(doc);
}

bool addClientToAuthList(PsychicHttpWebSocketConnection *connection)
{
  byte i;
  for (i=0; i<YB_CLIENT_LIMIT; i++)
  {
    //did we find an empty slot?
    if (!authenticatedConnections[i])
    {
      authenticatedConnections[i] = connection->id();
      break;
    }

    //are we already authenticated?
    if (authenticatedConnections[i] == connection->id())
      break;
  }

  //did we not find a spot?
  if (i == YB_CLIENT_LIMIT)
  {
    return false;
    Serial.println("ERROR: max clients reached");
  }
  else
   return true;
}