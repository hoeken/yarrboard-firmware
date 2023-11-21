#include "server.h"

PsychicHttpServer server;

// Variable to hold the last modification datetime
char last_modified[50];

CircularBuffer<WebsocketRequest*, YB_RECEIVE_BUFFER_COUNT> wsRequests;

int authenticatedConnections[YB_CLIENT_LIMIT];

String server_cert;
String server_key;

void server_setup()
{
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
    {
      server_cert = fp.readString();

      Serial.println("Server Cert:");
      Serial.println(server_cert);
    }
    else
    {
      Serial.println("server.pem not found, SSL not available");
      app_enable_ssl = false;
    }
    fp.close();

    File fp2 = LittleFS.open("/server.key");
    if (fp2)
    {
      server_key = fp2.readString();

      Serial.println("Server Key:");
      Serial.println(server_key);
    }
    else
    {
      Serial.println("server.key not found, SSL not available");
      app_enable_ssl = false;
    }
    fp2.close();
  }

  //do we want secure or not?
  if (app_enable_ssl)
    server.listen(443, server_cert.c_str(), server_key.c_str());
  else
    server.listen(80);

  //lets gooo!
  server.start();

  server.on("/", HTTP_GET, [](PsychicHttpServerRequest *request) {
    //TODO: how to make this work?
    //Check if the client already has the same version and respond with a 304 (Not modified)
    // if (strcmp(request->headers("If-Modified-Since").equals(last_modified)) {
    //     request->send(304);
    // } else {
        PsychicHttpServerResponse *response = request->beginResponse();
        response->setCode(200);
        response->setContentType("text/html");

        // Tell the browswer the contemnt is Gzipped
        response->addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response->addHeader("Last-Modified", last_modified);

        //add our actual content
        response->setContent(index_html_gz, index_html_gz_len);

        request->send(response);

        return ESP_OK;
  //  }
  });

  // Test the stream response class
  server.websocket("/ws")->
    onConnect([](PsychicHttpWebSocketConnection *connection) {
      Serial.printf("[socket] new connection (id %u)\n", connection->getConnection());
      return ESP_OK;
    })->
    // onClose([](PsychicHttpServerRequest *c) {
    //   PsychicHttpWebSocketConnection *connection = static_cast<PsychicHttpWebSocketConnection *>(c);
    //   Serial.println("[socket] connection closed");

    //   //stop tracking the connection
    //   if (require_login)
    //     for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    //       if (authenticatedConnections[i] == connection)
    //         authenticatedConnections[i] = NULL;
    // })->
    onFrame([](PsychicHttpWebSocketConnection *connection, httpd_ws_frame *frame) {
      handleWebSocketMessage(connection, frame->payload, frame->len);
      return ESP_OK;
    });

  //our main api connection
  server.on("/api/endpoint", HTTP_POST, [](PsychicHttpServerRequest *request)
  {
    StaticJsonDocument<1024> json;

    String body = request->body();
    DeserializationError err = deserializeJson(json, body);
    handleWebServerRequest(json, request);

    return ESP_OK;
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

    PsychicHttpServerResponse *response = request->beginResponse();
    response->setContentType("text/plain");

    if (LittleFS.exists("/coredump.txt"))
    {
      response->setCode(200);

      //this feels a little bit hacky...
      File fp = LittleFS.open("/coredump.txt");
      String data = fp.readString();
      response->setContent(data.c_str());
    }
    else
    {
      response->setCode(404);
      response->setContent("Coredump not found.");
    }

    request->send(response);

    return ESP_OK;
  });

  // server.on("/redirect", HTTP_GET, [](PsychicHttpServerRequest *request)
  // {
  //   request->redirect("/");

  //   return ESP_OK;
  // });

  // server.on("/auth-basic", HTTP_GET, [](PsychicHttpServerRequest *request)
  // {
  //   if (!request->authenticate(app_user, app_pass))
  //   {
  //     request->requestAuthentication(BASIC_AUTH, board_name, "You must log in.");
  //     return ESP_OK;
  //   }

  //   request->send("Success!");
  //   return ESP_OK;
  // });

  // server.on("/auth-digest", HTTP_GET, [](PsychicHttpServerRequest *request)
  // {
  //   if (!request->authenticate(app_user, app_pass))
  //   {
  //     request->requestAuthentication(DIGEST_AUTH, board_name, "You must log in.");
  //     return ESP_OK;
  //   }

  //   request->send("Success!");
  //   return ESP_OK;
  // });

  // server.on("/cookies", HTTP_GET, [](PsychicHttpServerRequest *request)
  // {
  //   PsychicHttpServerResponse *response = request->beginResponse();

  //   int counter = 0;
  //   if (request->hasCookie("counter"))
  //   {
  //     counter = std::stoi(request->getCookie("counter").c_str());
  //     counter++;
  //   }

  //   char cookie[10];
  //   sprintf(cookie, "%i", counter);

  //   response->setCookie("counter", cookie);
  //   response->setContent(cookie);
  //   request->send(response);

  //   return ESP_OK;
  // });
}

void server_loop()
{
  // //process our websockets outside the callback.
  // if (!wsRequests.isEmpty())
  //   handleWebsocketMessageLoop(wsRequests.shift());
}

void sendToAllWebsockets(const char * jsonString)
{
  //send the message to all authenticated clients.
  // if (require_login)
  // {
  //   for (byte i=0; i<YB_CLIENT_LIMIT; i++)
  //     if (authenticatedConnections[i])
  //     {

  //       //TODO: figure out how to do this.
  //       //authenticatedConnections[i]->send(jsonString);
  //     }
  // }
  // //nope, just sent it to all.
  // else
    server.sendAll(jsonString);
}

bool logClientIn(PsychicHttpWebSocketConnection *connection)
{
  //did we not find a spot?
  if (!addClientToAuthList(connection))
  {
    //TODO: is there a way to close a connection?
    //TODO: we need to test the max connections, mongoose may just handle it for us.
    // AsyncWebSocketClient* client = ws.client(client_id);
    // client->close();

    return false;
  }

  return true;
}

void handleWebServerRequest(JsonVariant input, PsychicHttpServerRequest *request)
{
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
    PsychicHttpServerResponse *response = request->beginResponse();
    response->setContentType("application/json");

    String jsonBuffer;
    serializeJson(output.as<JsonObject>(), jsonBuffer);
    response->setContent(jsonBuffer.c_str());
    request->send(response);
  }
  //give them valid json at least
  else
    request->send(200, "application/json", "{}");
}

void handleWebSocketMessage(PsychicHttpWebSocketConnection *connection, uint8_t *data, size_t len)
{
  char jsonBuffer[YB_MAX_JSON_LENGTH];
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);
  DynamicJsonDocument input(1024);

  //was there a problem, officer?
  DeserializationError err = deserializeJson(input, data);
  if (err)
  {
    char error[256];
    sprintf(error, "deserializeJson() failed with: %s", err.c_str());
    generateErrorJSON(output, error);
  }
  else
    handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, connection);

  //empty messages are valid, so don't send a response
  if (output.size())
  {
    serializeJson(output, jsonBuffer);
    connection->send(jsonBuffer);
  }

  /* trying the direct message handling now */
  // if (!wsRequests.isFull())
  // {
  //   WebsocketRequest* wr = new WebsocketRequest;
  //   wr->client = connection;
  //   strlcpy(wr->buffer, (char *)data, YB_RECEIVE_BUFFER_LENGTH); 
  //   wsRequests.push(wr);
  // }

  // //start throttling a little bit early so we don't miss anything
  // if (wsRequests.capacity <= YB_RECEIVE_BUFFER_COUNT/2)
  // {
  //   StaticJsonDocument<128> output;
  //   String jsonBuffer;
  //   generateErrorJSON(output, "Websocket busy, throttle connection.");
  //   serializeJson(output, jsonBuffer);

  //   connection->send(jsonBuffer.c_str());
  // }
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  // char jsonBuffer[YB_MAX_JSON_LENGTH];
  // DynamicJsonDocument output(YB_LARGE_JSON_SIZE);
  // DynamicJsonDocument input(1024);

  // //was there a problem, officer?
  // DeserializationError err = deserializeJson(input, request->buffer);
  // if (err)
  // {
  //   char error[64];
  //   sprintf(error, "deserializeJson() failed with code %s", err.c_str());
  //   generateErrorJSON(output, error);
  // }
  // else
  //   handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, request->client);

  // //empty messages are valid, so don't send a response
  // if (output.size())
  // {
  //   serializeJson(output, jsonBuffer);
  //   request->client->send(jsonBuffer);
  // }

  // delete request;
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
    if (authenticatedConnections[i] == connection->getConnection())
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
      authenticatedConnections[i] = connection->getConnection();
      break;
    }

    //are we already authenticated?
    if (authenticatedConnections[i] == connection->getConnection())
      break;
  }

  //did we not find a spot?
  if (i == YB_CLIENT_LIMIT)
    return false;
  else
   return true;
}