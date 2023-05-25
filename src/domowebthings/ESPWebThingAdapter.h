#pragma once

#include <ArduinoJson.h>
#ifndef ESP32
#include <ESP8266WebServerSecure.h>
#include "../bearssl/WebSockets4WebServerSecure.h"
#else
#include <WebSocketsClient.h>
#endif


#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif

#include "Thing.h"
#include "secutils.h"

#define ESP_MAX_PUT_BODY_SIZE 512

#ifndef LARGE_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define LARGE_JSON_DOCUMENT_SIZE 4096
#else
#define LARGE_JSON_DOCUMENT_SIZE 1024
#endif
#endif

#ifndef SMALL_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define SMALL_JSON_DOCUMENT_SIZE 1024
#else
#define SMALL_JSON_DOCUMENT_SIZE 256
#endif
#endif

#ifdef SHELLY_1_PLUS
/*
const char ENDPOINT_CA_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID1TCCAr2gAwIBAgIUEa2QL614B5+dSWJWXlEkDAEpeNYwDQYJKoZIhvcNAQEL
BQAweTELMAkGA1UEBhMCSVQxDjAMBgNVBAgMBUl0YWx5MQ0wCwYDVQQHDARQaXNh
MQ0wCwYDVQQKDAREb01PMQwwCgYDVQQLDANkZXYxDTALBgNVBAMMBERvTU8xHzAd
BgkqhkiG9w0BCQEWEGRldkBkb21vLWlvdC5jb20wIBcNMjIxMDE3MDcxODQyWhgP
MjA2MjEwMDcwNzE4NDJaMHkxCzAJBgNVBAYTAklUMQ4wDAYDVQQIDAVJdGFseTEN
MAsGA1UEBwwEUGlzYTENMAsGA1UECgwERG9NTzEMMAoGA1UECwwDZGV2MQ0wCwYD
VQQDDAREb01PMR8wHQYJKoZIhvcNAQkBFhBkZXZAZG9tby1pb3QuY29tMIIBIjAN
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxi8P20LvbGxuY1St1taYBV967Zrc
3AaAZ4SkOWCnjSOPfuAqlg1rA0y+i6WQ+dzxwIdyWxx8CIyawPXefy2JxLA/wATu
Gvw8pOMl0DSKrYO6TCgGrtQOvpD4jlzvxKK4k3YdqDESD46484Ubs6LgjqvDwr0Q
CxA6LOWYsChErqqRZR1wTxRcMGolEjudTJiu+27ZHQECcoFovd09JLc7x/K/rTJT
XjaP4cFY+GO3sDGj7Mmy0X053d5iH7MHmPsb/5M8XxvqGHqCrk0QC8CzJHl2y/1H
GMxKq41LxZ5iBnplfCVO+gfqSLP1Trmt0I6oUf/LdkzjNUzLUnCX5JqcCwIDAQAB
o1MwUTAdBgNVHQ4EFgQUj8AHLRUCFuC9i/x1JKAK5p7SoiIwHwYDVR0jBBgwFoAU
j8AHLRUCFuC9i/x1JKAK5p7SoiIwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAQEAOaBL0Uc1EjXCs3dMi88IVMlvMSJ7dWejiLboLMavBVsnxBj86w5j
TiqLB8AZaqTpob7/dbDHiAUKwGtGTEBCHi/gmr5Y2ASoj/20tw0Zzmq5UGbsB9Oz
5mv7nWxJY2efkQY5XHT8U41rQ0zy7lwhhCANFSwOk/KpzLaKuhiI9eYUPZxqTTLz
MxPvY2UMKsCSpN8zVFdhLb3+aFDblH6kDHs/wLFcOGXAYsnREcHqx4xia7i5beGp
o8JlNiGB3Hp1dbk2lNTTRPhKUduGJ16Y+2FnWrti47vWWEKFiepB4CXT7r76vsr3
Sc0E93ZazfW4BuTNUqKxLqPONg+InTGcrw==
-----END CERTIFICATE-----
)EOF";
*/
#endif

class WebThingAdapter {
public:
  WebThingAdapter(String _name, IPAddress _ip, uint16_t _port)
      : name(_name), ip(_ip.toString()), port(_port) {
        #ifndef ESP32
        this->server = new BearSSL::ESP8266WebServerSecure(port);
        this->webSocket = new WebSockets4WebServerSecure;
        #else
        this->webSocket = new WebSocketsClient();
        #endif
  }

  void begin() {

    #ifndef ESP32
    name.toLowerCase();
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("webthing", "tcp", port);
    MDNS.addServiceTxt("webthing", "tcp", "path", "/");

    if(!getDomoSecMaterial(this->serverCert, this->serverKey, this->authUser, this->authPassword)){
        Serial.println("SecMaterial not found, reset !!!");
        Serial.flush();

        ESP.restart();
        delay(1000);
        ESP.reset();
    }

    this->server->getServer().setRSACert(new BearSSL::X509List(serverCert.c_str()), new BearSSL::PrivateKey(serverKey.c_str()));

    this->server->getServer().setBufferSizes(1024,1024);

    this->server->begin();

    this->webSocket->setAuthorization(authUser.c_str(), authPassword.c_str());

    Serial.println("Server is ready");

    Serial.println("name: " + this->name);

    Serial.println("ip: " + this->ip);

    Serial.flush();

    #else

    if(!getDomoClientSecMaterial(this->caCert, this->authUser, this->authPassword)){
          Serial.println("SecMaterial not found, reset !!!");
          Serial.flush();

          ESP.restart();
          delay(1000);
          ESP.restart();
    }

    Serial.println("Starting websocket secure client");
    Serial.flush();

    this->webSocket->beginSslWithCA("domoserver", 5000, "/", this->caCert.c_str());
    this->webSocket->setAuthorization(this->authUser.c_str(), this->authPassword.c_str());

    Serial.println("websocket begin");
    Serial.flush();

    #endif
  }

  void update() {

    #ifndef ESP32
    this->server->handleClient();
    this->webSocket->loop();
    MDNS.update();
    #else

    this->webSocket->loop();

    #endif


    // * Send changed properties as defined in "4.5 propertyStatus message"
    // Do this by looping over all devices and properties
    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      sendChangedProperties(device);
      device = device->next;
    }
  }

  void addDevice(ThingDevice *device) {
    if (this->lastDevice == nullptr) {
      this->firstDevice = device;
      this->lastDevice = device;
    } else {
      this->lastDevice->next = device;
      this->lastDevice = device;
    }

    #ifndef ESP32
    this->server->addHook(webSocket->hookForWebserver("/things/" + device->id,
                                                      std::bind(&WebThingAdapter::webSocketEvent,
                                                                this, std::placeholders::_1,
                                                                std::placeholders::_2,
                                                                std::placeholders::_3,
                                                                std::placeholders::_4
                                                                )));
    device->ws = webSocket;
    #else
    this->webSocket->onEvent(std::bind(&WebThingAdapter::webSocketEvent,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3 ));
    device->ws = webSocket;
    #endif
  }

private:

  #ifndef ESP32
  BearSSL::ESP8266WebServerSecure* server;
  WebSockets4WebServerSecure* webSocket;
  String serverCert;
  String serverKey;
  String authUser;
  String authPassword;
  #else

  String serverAddress;
  WebSocketsClient* webSocket;
  String caCert;
  String authUser;
  String authPassword;
  #endif


  String name;
  String ip;
  uint16_t port;


  ThingDevice *firstDevice = nullptr;
  ThingDevice *lastDevice = nullptr;
  char body_data[ESP_MAX_PUT_BODY_SIZE];
  bool b_has_body_data = false;



  void sendChangedProperties(ThingDevice *device) {

    // Prepare one buffer per device
    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
    message["messageType"] = "propertyStatus";
    JsonObject prop = message.createNestedObject("data");
    bool dataToSend = false;
    ThingItem *item = device->firstProperty;
    while (item != nullptr) {
      ThingDataValue *value = item->changedValueOrNull();
      if (value) {
        dataToSend = true;
        item->serializeValue(prop);
      }
      item = item->next;
    }
    if (dataToSend) {
      String jsonStr;
      serializeJson(message, jsonStr);
      #ifndef ESP32
      // Inform all connected ws clients of a Thing about changed properties
      ((WebSockets4WebServerSecure *)device->ws)->broadcastTXT(jsonStr);
      #else
      Serial.println("Sending text");
      Serial.flush();
      device->ws->sendTXT(jsonStr);
      #endif
    }
  }

  #ifndef ESP32
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t * rawData, size_t len) {

        auto device = this->firstDevice;


        if (type == WStype_DISCONNECTED || type == WStype_ERROR) {
            this->webSocket->disconnect();
            return;
        }

        // Web Thing only specifies text, not binary websocket transfers
        if (type != WStype_TEXT)
            return;

      Serial.println( (char*)rawData);
      Serial.flush();


        // Parse request
        DynamicJsonDocument newProp(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newProp, rawData, len);
        if (error) {
            Serial.println("Invalid json");
            return;
        }



        String messageType = newProp["messageType"].as<String>();
        JsonVariant dataVariant = newProp["data"];
        if (!dataVariant.is<JsonObject>()) {
            Serial.println("data must be an object");
            return;
        }

        JsonObject data = dataVariant.as<JsonObject>();

        if (messageType == "setProperty") {
            for (JsonPair kv : data) {
                device->setProperty(kv.key().c_str(), kv.value());
            }
        } else if (messageType == "requestAction") {


            for (JsonPair kv : data) {
                DynamicJsonDocument *actionRequest =
                        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);

                JsonObject actionObj = actionRequest->to<JsonObject>();
                JsonObject nested = actionObj.createNestedObject(kv.key());

                for (JsonPair kvInner : kv.value().as<JsonObject>()) {
                    nested[kvInner.key()] = kvInner.value();
                }

                ThingActionObject *obj = device->requestAction(actionRequest);
                if (obj != nullptr) {
                    obj->setNotifyFunction(std::bind(&ThingDevice::sendActionStatus,
                                                     device, std::placeholders::_1));
                    device->sendActionStatus(obj);

                    obj->start();

                }
            }
        }

  }
  #else

  void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
      auto device = this->firstDevice;

      if (type == WStype_DISCONNECTED || type == WStype_ERROR) {
          this->webSocket->disconnect();
          return;
      }

      // Web Thing only specifies text, not binary websocket transfers
      if (type != WStype_TEXT)
          return;

      Serial.println( (char*)payload);
      Serial.flush();


      // Parse request
      DynamicJsonDocument newProp(SMALL_JSON_DOCUMENT_SIZE);
      auto error = deserializeJson(newProp, payload, length);
      if (error) {
          Serial.println("Invalid json");
          return;
      }



      String messageType = newProp["messageType"].as<String>();
      JsonVariant dataVariant = newProp["data"];
      if (!dataVariant.is<JsonObject>()) {
          Serial.println("data must be an object");
          return;
      }

      JsonObject data = dataVariant.as<JsonObject>();

      if (messageType == "setProperty") {
          for (JsonPair kv : data) {
              device->setProperty(kv.key().c_str(), kv.value());
          }
      } else if (messageType == "requestAction") {


          for (JsonPair kv : data) {
              DynamicJsonDocument *actionRequest =
                      new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);

              JsonObject actionObj = actionRequest->to<JsonObject>();
              JsonObject nested = actionObj.createNestedObject(kv.key());

              for (JsonPair kvInner : kv.value().as<JsonObject>()) {
                  nested[kvInner.key()] = kvInner.value();
              }

              ThingActionObject *obj = device->requestAction(actionRequest);
              if (obj != nullptr) {
                  obj->setNotifyFunction(std::bind(&ThingDevice::sendActionStatus,
                                                   device, std::placeholders::_1));
                  device->sendActionStatus(obj);

                  obj->start();

              }
          }
      }

  }

  #endif

};

