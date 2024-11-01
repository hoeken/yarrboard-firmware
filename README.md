# Yarrboard Firmware

This is the firmware that runs on a [Yarrboard](https://github.com/hoeken/yarrboard) compatible board.  Current boards include:

* [FrothFET - 8 Channel Digital Switching](https://github.com/hoeken/frothfet)
* [Brineomatic - Watermaker Controller](https://github.com/hoeken/brineomatic)

The project is based around the amazing esp32 and mainly supports boards based around that.

Pronounced *yarrrrrrr * bird*.  Lots of r's and don't call it a board. ;)

## Installation and Setup (dev mode)

* Clone this repository
* Run ```npm install``` in the repository to get some dev tools
* Plug your computer into the board
* Open the repository in VSCode (you need Platformio plugin too)
* At the bottom, select your board from the build environments
* Upload the firmware

### First Time Connection

* Connect to Yarrboard wifi
* Open browser to http://yarrboard.local or http://yarrboard
* Enter your boat wifi credentials on the configuration page
* Reconnect to your boat wifi
* Open browser to http://yarrboard.local or http://yarrboard
* Update board settings, such as login info, channel names, soft fuses, etc.
* Install SignalK + signalk-yarrboard-plugin and configure
* Setup any Node-RED flows and custom logic you want.

## Protocol

The protocol for communicating with Yarrboard is entirely based on JSON. Each request to the server should be a single JSON object, and the server will respond with a JSON object.

Here are some example commands you could send:

```
{"cmd":"ping"}
{"cmd":"get_config","value":true,"user":"admin","pass":"admin"}
{"cmd":"set_channel","id":0,"state":true,"user":"admin","pass":"admin"}
{"cmd":"login","user":"admin","pass":"admin"}
{"cmd":"set_channel","id":0,"state":true}
{"cmd":"set_channel","id":0,"duty":0.99}
{"cmd":"set_channel","id":0,"duty":0.5}
{"cmd":"set_channel","id":0,"duty":0.1}
{"cmd":"set_channel","id":0,"state":false}
```

### Websockets Protocol

Yarrboard provides a websocket server on **http://yarrboard.local/ws**

JSON is sent and received as text.  Clients communicating over websockets can send a **login** command, or include your username and password with each request.

### Web API Protocol

Yarrboard provides a POST API endpoint at **http://yarrboard.local/api/endpoint**

Examples of how to communicate with this endpoint:

```
curl -i -d '{"cmd":"ping"}'  -H "Content-Type: application/json"  -X POST http://yarrboard.local/api/endpoint
curl -i -d '{"cmd":"set_channel","user":"admin","pass":"admin","id":0,"state":true}'  -H "Content-Type: application/json"  -X POST http://yarrboard.local/api/endpoint
```
Note, you will need to pass your username/password in each request with the Web API.

Additionally, there are a few convenience urls to get basic info.  These are GET only and optionally accept the **user** and **pass** parameters.

* http://yarrboard.local/api/config
* http://yarrboard.local/api/stats
* http://yarrboard.local/api/update

Some example code:

```
curl -i -X GET http://yarrboard.local/api/config
curl -i -X GET 'http://yarrboard.local/api/config?user=admin&pass=admin'
curl -i -X GET 'http://yarrboard.local/api/stats?user=admin&pass=admin'
curl -i -X GET 'http://yarrboard.local/api/update?user=admin&pass=admin'
```

### Serial API Protocol

Yarrboard provides a USB serial port that communicates at 115200 baud.  It uses the same JSON protocol as websockets and the web API.

Clients communicating over serial can send a **login** command, or include your username and password with each request.

Each command should end with a newline (\n) and each response will end with a newline (\n).

This port is also used for debugging, so make sure you check that each line parses into valid JSON before you try to use it.
