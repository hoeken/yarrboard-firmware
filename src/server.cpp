#include "server.h"

PsychicHttpsServer server;
PsychicWebSocketHandler websocketHandler;

// Variable to hold the last modification datetime
char last_modified[50];

QueueHandle_t wsRequests;

AuthenticatedClient authenticatedClients[YB_CLIENT_LIMIT];

String server_cert;
String server_key;

void server_setup()
{
  //init our authentication stuff
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
  {
    authenticatedClients[i].socket = 0;
    authenticatedClients[i].role = app_default_role;
  }

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

  server.on("/", HTTP_GET, [](PsychicRequest *request) {

    //Check if the client already has the same version and respond with a 304 (Not modified)
    if (request->header("If-Modified-Since").indexOf(last_modified) > 0)
      return request->reply(304);
    //What about our ETag?
    else if (request->header("If-None-Match").equals(index_html_gz_sha))
      return request->reply(304);
    else
    {
        PsychicResponse response(request);
        response.setCode(200);
        response.setContentType("text/html");

        // Tell the browswer the contemnt is Gzipped
        response.addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response.addHeader("Last-Modified", last_modified);
        response.addHeader("ETag", index_html_gz_sha);
        //response.addHeader("Cache-Control", "max-age=604800"); // cache for 1 week

        //add our actual content
        response.setContent(index_html_gz, index_html_gz_len);

        return response.send();
    }
  });

  server.on("/logo-navico.png", HTTP_GET, [](PsychicRequest *request)
  {
    PsychicResponse response(request);
    response.setCode(200);
    response.setContentType("image/png");

    // Tell the browswer the contemnt is Gzipped
    response.addHeader("Content-Encoding", "gzip");

    // And set the last-modified datetime so we can check if we need to send it again next time or not
    response.addHeader("Last-Modified", last_modified);
    response.addHeader("ETag", logo_navico_gz_sha);

    //add our actual content
    response.setContent(logo_navico_gz, logo_navico_gz_len);

    return response.send();
  });

  // Our websocket handler
  websocketHandler.onFrame([](PsychicWebSocketRequest *request, httpd_ws_frame *frame) {
    handleWebSocketMessage(request, frame->payload, frame->len);
    return ESP_OK;
  });
  websocketHandler.onOpen([](PsychicWebSocketClient *client) {
    Serial.printf("[socket] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString());
    websocketClientCount++;
  });
  websocketHandler.onClose([](PsychicWebSocketClient *client) {
    Serial.printf("[socket] connection #%u closed from %s\n", client->socket(), client->remoteIP().toString());
    websocketClientCount--;

    removeClientFromAuthList(client);
  });
  server.on("/ws", &websocketHandler);

  server.onOpen([](PsychicClient *client) {
    httpClientCount++;
  });

  server.onClose([](PsychicClient *client) {
    httpClientCount--;
  });


  //our main api connection
  server.on("/api/endpoint", HTTP_POST, [](PsychicRequest *request)
  {
    StaticJsonDocument<1024> json;

    String body = request->body();
    DeserializationError err = deserializeJson(json, body);
    return handleWebServerRequest(json, request);
  });

  //send config json
  server.on("/api/config", HTTP_GET, [](PsychicRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_config";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //send stats json
  server.on("/api/stats", HTTP_GET, [](PsychicRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_stats";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //send update json
  server.on("/api/update", HTTP_GET, [](PsychicRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_update";

    handleWebServerRequest(json, request);

    return ESP_OK;
  });

  //downloadable coredump file
  server.on("/coredump.txt", HTTP_GET, [](PsychicRequest *request)
  {
    //delete the coredump here, but not from littlefs
		deleteCoreDump();

    //dont bug the client anymore
    has_coredump = false;

    PsychicResponse response(request);
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

void sendToAllWebsockets(const char * jsonString, UserRole auth_level)
{
  //make sure we're allowed to see the message
  if (auth_level > app_default_role)
  {
    for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    {
      if (authenticatedClients[i].socket)
      {
        //make sure its a valid client
        PsychicWebSocketClient *client = websocketHandler.getClient(authenticatedClients[i].socket);
        if (client == NULL)
          continue;

        if (authenticatedClients[i].role >= auth_level)
          client->sendMessage(jsonString);
      }
    }
  }
  //nope, just sent it to all.
  else
    websocketHandler.sendAll(jsonString);
}

bool logClientIn(PsychicWebSocketClient *client, UserRole role)
{
  //did we not find a spot?
  if (!addClientToAuthList(client, role))
  {
    Serial.println("Error: could not add to auth list.");

    //i'm pretty sure this closes our connection
    close(client->socket());

    return false;
  }

  return true;
}

esp_err_t handleWebServerRequest(JsonVariant input, PsychicRequest *request)
{
  esp_err_t err = ESP_OK;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  if (request->hasParam("user"))
    input["user"] = request->getParam("user")->value();
  if (request->hasParam("pass"))
    input["pass"] = request->getParam("pass")->value();

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
      PsychicResponse response(request);
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

void handleWebSocketMessage(PsychicWebSocketRequest *request, uint8_t *data, size_t len)
{
  //build our websocket request - copy the existing one
  //we are allocating memory here, and the worker will free it
  WebsocketRequest wr;
  wr.socket = request->client()->socket();
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
    request->client()->close();
    Serial.printf("[socket] queue full #%d\n", wr.socket);

    //free the memory... no worker to do it for us.
    free(wr.buffer);
  }

  //send a throttle message if we're full
  // if (!uxQueueSpacesAvailable(wsRequests))
  // {
  //   StaticJsonDocument<128> output;

  //   //dynamically allocate our buffer
  //   size_t jsonSize = measureJson(output);
  //   char * jsonBuffer = (char *)malloc(jsonSize+1);
  //   jsonBuffer[jsonSize] = '\0'; // null terminate

  //   //did we get anything?
  //   if (jsonBuffer != NULL)
  //   {
  //     generateErrorJSON(output, "Queue Full");
  //     serializeJson(output, jsonBuffer, jsonSize+1);
  //     request->reply(jsonBuffer);
  //   }
  //   free(jsonBuffer);
  // }
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  //make sure our client is still good.
  PsychicWebSocketClient *client = websocketHandler.getClient(request->socket);
  if (client == NULL)
  {
    //Serial.printf("[socket] client #%d bad, bailing\n", request->socket);
    return;
  }

  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);
  DynamicJsonDocument input(1024);

  //was there a problem, officer?
  DeserializationError err = deserializeJson(input, request->buffer);
  if (err)
  {
    char error[64];
    sprintf(error, "deserializeJson() failed with code %s", err.c_str());
    generateErrorJSON(output, error);
  }
  else
    handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, client);

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
      client->sendMessage(jsonBuffer);

      //keep track!
      sentMessages++;
      totalSentMessages++;
    }

    free(jsonBuffer);    
  }
}

bool isLoggedIn(JsonVariantConst input, byte mode, PsychicWebSocketClient *connection)
{
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

UserRole getUserRole(JsonVariantConst input, byte mode, PsychicWebSocketClient *connection)
{
  //login only required for websockets.
  if (mode == YBP_MODE_WEBSOCKET)
    return getWebsocketRole(input, connection);
  else if (mode == YBP_MODE_HTTP)
    return api_role;
  else if (mode == YBP_MODE_SERIAL)
    return serial_role;
  else
    return app_default_role;

}

bool isWebsocketClientLoggedIn(JsonVariantConst doc, PsychicWebSocketClient *client)
{
  //are they in our auth array?
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    if (authenticatedClients[i].socket == client->socket())
      return true;

  return false;
}

UserRole getWebsocketRole(JsonVariantConst doc, PsychicWebSocketClient *client)
{
  //are they in our auth array?
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    if (authenticatedClients[i].socket == client->socket())
      return authenticatedClients[i].role;

  return app_default_role;
}


bool checkLoginCredentials(JsonVariantConst doc, UserRole &role)
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
  if (!strcmp(admin_user, myuser) && !strcmp(admin_pass, mypass))
  {
    role = ADMIN;
    return true;
  }

  if (!strcmp(guest_user, myuser) && !strcmp(guest_pass, mypass))
  {
    role = GUEST;
    return true;
  }

  //default to fail then.
  role = app_default_role;
  return false;  
}

bool isSerialClientLoggedIn(JsonVariantConst doc)
{
  if (is_serial_authenticated)
    return true;
  else
    return checkLoginCredentials(doc, serial_role);
}

bool isApiClientLoggedIn(JsonVariantConst doc)
{
  return checkLoginCredentials(doc, api_role);
}

bool addClientToAuthList(PsychicWebSocketClient *client, UserRole role)
{
  byte i;
  for (i=0; i<YB_CLIENT_LIMIT; i++)
  {
    //did we find an empty slot?
    if (!authenticatedClients[i].socket)
    {
      authenticatedClients[i].socket = client->socket();
      authenticatedClients[i].role = role;
      break;
    }

    //are we already authenticated?
    if (authenticatedClients[i].socket == client->socket())
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

void removeClientFromAuthList(PsychicWebSocketClient *client)
{
  byte i;
  for (i=0; i<YB_CLIENT_LIMIT; i++)
  {
    //did we find an empty slot?
    if (authenticatedClients[i].socket == client->socket())
    {
      authenticatedClients[i].socket = 0;
      authenticatedClients[i].role = app_default_role;
    }
  }
}