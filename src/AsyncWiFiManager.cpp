/**
 * AsyncWiFiManager.cpp
 * 
 * AsyncWiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Creator lbussy
 * @author lbussy
 * @version 0.0.0
 * @license MIT
 */

#include "AsyncWiFiManager.h"

#if defined(ESP8266) || defined(ESP32)

#ifdef ESP32
uint8_t AsyncWiFiManager::_lastconxresulttmp = WL_IDLE_STATUS;
#endif

/**
 * --------------------------------------------------------------------------------
 *  AsyncWiFiManagerParameter
 * --------------------------------------------------------------------------------
**/

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter()
{
    AsyncWiFiManagerParameter("");
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *custom)
{
    _id = NULL;
    _label = NULL;
    _length = 1;
    _value = NULL;
    _labelPlacement = AWFM_LABEL_BEFORE;
    _customHTML = custom;
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *label)
{
    init(id, label, "", 0, "", AWFM_LABEL_BEFORE);
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length)
{
    init(id, label, defaultValue, length, "", AWFM_LABEL_BEFORE);
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom)
{
    init(id, label, defaultValue, length, custom, AWFM_LABEL_BEFORE);
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
    init(id, label, defaultValue, length, custom, labelPlacement);
}

void AsyncWiFiManagerParameter::init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
    _id = id;
    _label = label;
    _labelPlacement = labelPlacement;
    _customHTML = custom;
    setValue(defaultValue, length);
}

AsyncWiFiManagerParameter::~AsyncWiFiManagerParameter()
{
    if (_value != NULL)
    {
        delete[] _value;
    }
    _length = 0; // setting length 0, ideally the entire parameter should be removed, or added to wifimanager scope so it follows
}

// AsyncWiFiManagerParameter& AsyncWiFiManagerParameter::operator=(const AsyncWiFiManagerParameter& rhs){
//   Serial.println("copy assignment op called");
//   (*this->_value) = (*rhs._value);
//   return *this;
// }

// @note debug is not available in wmparameter class
void AsyncWiFiManagerParameter::setValue(const char *defaultValue, int length)
{
    if (!_id)
    {
        // Serial.println("cannot set value of this parameter");
        return;
    }

    // if(strlen(defaultValue) > length){
    //   // Serial.println("defaultValue length mismatch");
    //   // return false; //@todo bail
    // }

    _length = length;
    _value = new char[_length + 1];
    memset(_value, 0, _length + 1); // explicit null

    if (defaultValue != NULL)
    {
        strncpy(_value, defaultValue, _length);
    }
}
const char *AsyncWiFiManagerParameter::getValue()
{
    // Serial.println(printf("Address of _value is %p\n", (void *)_value));
    return _value;
}
const char *AsyncWiFiManagerParameter::getID()
{
    return _id;
}
const char *AsyncWiFiManagerParameter::getPlaceholder()
{
    return _label;
}
const char *AsyncWiFiManagerParameter::getLabel()
{
    return _label;
}
int AsyncWiFiManagerParameter::getValueLength()
{
    return _length;
}
int AsyncWiFiManagerParameter::getLabelPlacement()
{
    return _labelPlacement;
}
const char *AsyncWiFiManagerParameter::getCustomHTML()
{
    return _customHTML;
}

/**
 * [addParameter description]
 * @access public
 * @param {[type]} AsyncWiFiManagerParameter *p [description]
 */
bool AsyncWiFiManager::addParameter(AsyncWiFiManagerParameter *p)
{

    // check param id is valid, unless null
    if (p->getID())
    {
        for (size_t i = 0; i < strlen(p->getID()); i++)
        {
            if (!(isAlphaNumeric(p->getID()[i])) && !(p->getID()[i] == '_'))
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_ERROR, F("[ERROR] parameter IDs can only contain alpha numeric chars"));
#endif
                return false;
            }
        }
    }

    // init params if never malloc
    if (_params == NULL)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("allocating params bytes:"), _max_params * sizeof(AsyncWiFiManagerParameter *));
#endif
        _params = (AsyncWiFiManagerParameter **)malloc(_max_params * sizeof(AsyncWiFiManagerParameter *));
    }

    // resize the params array by increment of ASYNC_WIFI_MANAGER_MAX_PARAMS
    if (_paramsCount == _max_params)
    {
        _max_params += ASYNC_WIFI_MANAGER_MAX_PARAMS;
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("Updated _max_params:"), _max_params);
        DEBUG_AWM(DEBUG_DEV, F("re-allocating params bytes:"), _max_params * sizeof(AsyncWiFiManagerParameter *));
#endif
        AsyncWiFiManagerParameter **new_params = (AsyncWiFiManagerParameter **)realloc(_params, _max_params * sizeof(AsyncWiFiManagerParameter *));
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(ASYNC_WIFI_MANAGER_MAX_PARAMS);
// DEBUG_AWM(_paramsCount);
// DEBUG_AWM(_max_params);
#endif
        if (new_params != NULL)
        {
            _params = new_params;
        }
        else
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] failed to realloc params, size not increased!"));
#endif
            return false;
        }
    }

    _params[_paramsCount] = p;
    _paramsCount++;

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Added Parameter:"), p->getID());
#endif
    return true;
}

/**
 * [getParameters description]
 * @access public
 */
AsyncWiFiManagerParameter **AsyncWiFiManager::getParameters()
{
    return _params;
}

/**
 * [getParametersCount description]
 * @access public
 */
int AsyncWiFiManager::getParametersCount()
{
    return _paramsCount;
}

/**
 * --------------------------------------------------------------------------------
 *  AsyncWiFiManager 
 * --------------------------------------------------------------------------------
**/

// constructors
AsyncWiFiManager::AsyncWiFiManager(Stream &consolePort) : _debugPort(consolePort)
{
    AsyncWiFiManagerInit();
}

AsyncWiFiManager::AsyncWiFiManager()
{
    AsyncWiFiManagerInit();
}

void AsyncWiFiManager::AsyncWiFiManagerInit()
{
    setMenu(_menuIdsDefault);
    if (_debug && _debugLevel >= DEBUG_DEV)
        debugPlatformInfo();
    _max_params = ASYNC_WIFI_MANAGER_MAX_PARAMS;
}

// destructor
AsyncWiFiManager::~AsyncWiFiManager()
{
    _end();
    // parameters
    // @todo below belongs to wifimanagerparameter
    if (_params != NULL)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("freeing allocated params!"));
#endif
        free(_params);
        _params = NULL;
    }

// @todo remove event
// WiFi.onEvent(std::bind(&AsyncWiFiManager::WiFiEvent,this,_1,_2));
#ifdef ESP32
    WiFi.removeEvent(wm_event_id);
#endif

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("unloading"));
#endif
}

void AsyncWiFiManager::_begin()
{
    if (_hasBegun)
        return;
    _hasBegun = true;
    // _usermode = WiFi.getMode();

#ifndef ESP32
    WiFi.persistent(false); // disable persistent so scannetworks and mode switching do not cause overwrites
#endif
}

void AsyncWiFiManager::_end()
{
    _hasBegun = false;
    if (_userpersistent)
        WiFi.persistent(true); // reenable persistent, there is no getter we rely on _userpersistent
                               // if(_usermode != WIFI_OFF) WiFi.mode(_usermode);
}

// AUTOCONNECT

boolean AsyncWiFiManager::autoConnect()
{
    String ssid = getDefaultAPName();
    return autoConnect(ssid.c_str(), NULL);
}

/**
 * [autoConnect description]
 * @access public
 * @param  {[type]} char const         *apName     [description]
 * @param  {[type]} char const         *apPassword [description]
 * @return {[type]}      [description]
 */
boolean AsyncWiFiManager::autoConnect(char const *apName, char const *apPassword)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("AutoConnect"));
#endif
    if (getWiFiIsSaved())
    {

        _begin();

        // attempt to connect using saved settings, on fail fallback to AP config portal
        if (!WiFi.enableSTA(true))
        {
// handle failure mode Brownout detector etc.
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[FATAL] Unable to enable wifi!"));
#endif
            return false;
        }

        WiFiSetCountry();

#ifdef ESP32
        if (esp32persistent)
            WiFi.persistent(false); // disable persistent for esp32 after esp_wifi_start or else saves wont work
#endif

        _usermode = WIFI_STA; // When using autoconnect , assume the user wants sta mode on permanently.

        // no getter for autoreconnectpolicy before this
        // https://github.com/esp8266/Arduino/pull/4359
        // so we must force it on else, if not connectimeout then waitforconnectionresult gets stuck endless loop
        WiFi_autoReconnect();

        // set hostname before stating
        if ((String)_hostname != "")
        {
            setupHostname(true);
        }

        // if already connected, or try stored connect
        // @note @todo ESP32 has no autoconnect, so connectwifi will always be called unless user called begin etc before
        // @todo check if correct ssid == saved ssid when already connected
        bool connected = false;
        if (WiFi.status() == WL_CONNECTED)
        {
            connected = true;
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("AutoConnect: ESP Already Connected"));
#endif
            setSTAConfig();
            // @todo not sure if this check makes sense, causes dup setSTAConfig in connectwifi,
            // and we have no idea WHAT we are connected to
        }

        if (connected || connectWifi(_defaultssid, _defaultpass) == WL_CONNECTED)
        {
//connected
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("AutoConnect: SUCCESS"));
            DEBUG_AWM(F("STA IP Address:"), WiFi.localIP());
#endif
            _lastconxresult = WL_CONNECTED;

            if ((String)_hostname != "")
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_DEV, F("hostname: STA: "), getWiFiHostname());
#endif
            }
            return true; // connected success
        }

        // possibly skip the config portal
        if (!_enableConfigPortal)
        {
            return false; // not connected and not cp
        }

#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("AutoConnect: FAILED"));
#endif
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("No Credentials are Saved, skipping connect"));
#endif
    }

    // not connected start configportal
    bool res = startConfigPortal(apName, apPassword);
    return res;
}

bool AsyncWiFiManager::setupHostname(bool restart)
{
    if ((String)_hostname == "")
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("No Hostname to set"));
#endif
        return false;
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("setupHostname: "), _hostname);
#endif
    }
    bool res = true;
#ifdef ESP8266
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Setting WiFi hostname"));
#endif
    res = WiFi.hostname(_hostname);
// #ifdef ESP8266MDNS_H
#ifdef AWM_MDNS
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Setting MDNS hostname, tcp 80"));
#endif
    if (MDNS.begin(_hostname))
    {
        MDNS.addService("http", "tcp", 80);
    }
#endif
#elif defined(ESP32)
    // @note hostname must be set after STA_START
    delay(200); // do not remove, give time for STA_START
    res = WiFi.setHostname(_hostname);
    // #ifdef ESP32MDNS_H
#ifdef AWM_MDNS
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Setting MDNS hostname, tcp 80"));
#endif
    if (MDNS.begin(_hostname))
    {
        MDNS.addService("http", "tcp", 80);
    }
#endif
#endif

#ifdef AWM_DEBUG_LEVEL
    if (!res)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] hostname: set failed!"));
#endif

    if (restart && (WiFi.status() == WL_CONNECTED))
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("reconnecting to set new hostname"));
#endif
        // WiFi.reconnect(); // This does not reset dhcp
        WiFi_Disconnect();
        delay(200); // do not remove, need a delay for disconnect to change status()
    }

    return res;
}

// CONFIG PORTAL
bool AsyncWiFiManager::startAP()
{
    bool ret = true;
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("StartAP with SSID: "), _apName);
#endif

#ifdef ESP8266
    // @bug workaround for bug #4372 https://github.com/esp8266/Arduino/issues/4372
    if (!WiFi.enableAP(true))
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] enableAP failed!"));
#endif
        return false;
    }
    delay(500); // workaround delay
#endif

    // setup optional soft AP static ip config
    if (_ap_static_ip)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("Custom AP IP/GW/Subnet:"));
#endif
        if (!WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn))
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] softAPConfig failed!"));
#endif
        }
    }

    //@todo add callback here if needed to modify ap but cannot use setAPStaticIPConfig
    //@todo rework wifi channelsync as it will work unpredictably when not connected in sta

    int32_t channel = 0;
    if (_channelSync)
        channel = WiFi.channel();
    else
        channel = _apChannel;

    if (channel > 0)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Starting AP on channel:"), channel);
#endif
    }

    // start soft AP with password or anonymous
    // default channel is 1 here and in esplib, @todo just change to default remove conditionals
    if (_apPassword != "")
    {
        if (channel > 0)
        {
            ret = WiFi.softAP(_apName.c_str(), _apPassword.c_str(), channel, _apHidden);
        }
        else
        {
            ret = WiFi.softAP(_apName.c_str(), _apPassword.c_str(), 1, _apHidden); //password option
        }
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("AP has anonymous access!"));
#endif
        if (channel > 0)
        {
            ret = WiFi.softAP(_apName.c_str(), "", channel, _apHidden);
        }
        else
        {
            ret = WiFi.softAP(_apName.c_str(), "", 1, _apHidden);
        }
    }

    if (_debugLevel >= DEBUG_DEV)
        debugSoftAPConfig();

    // @todo add softAP retry here

    delay(500); // slight delay to make sure we get an AP IP
#ifdef AWM_DEBUG_LEVEL
    if (!ret)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] There was a problem starting the AP"));
    DEBUG_AWM(F("AP IP address:"), WiFi.softAPIP());
#endif

// set ap hostname
#ifdef ESP32
    if (ret && (String)_hostname != "")
    {
        bool res = WiFi.softAPsetHostname(_hostname);
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("setting softAP Hostname:"), _hostname);
        if (!res)
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] hostname: AP set failed!"));
        DEBUG_AWM(DEBUG_DEV, F("hostname: AP: "), WiFi.softAPgetHostname());
#endif
    }
#endif

    return ret;
}

/**
 * [startWebPortal description]
 * @access public
 * @return {[type]} [description]
 */
void AsyncWiFiManager::startWebPortal()
{
    if (configPortalActive || webPortalActive)
        return;
    setupConfigPortal();
    webPortalActive = true;
}

/**
 * [stopWebPortal description]
 * @access public
 * @return {[type]} [description]
 */
void AsyncWiFiManager::stopWebPortal()
{
    if (!configPortalActive && !webPortalActive)
        return;
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Stopping Web Portal"));
#endif
    webPortalActive = false;
    shutdownConfigPortal();
}

boolean AsyncWiFiManager::configPortalHasTimeout()
{
    if (!configPortalActive)
        return false;
    uint16_t logintvl = 30000; // how often to emit timeing out counter logging

    // handle timeout portal client check
    if (_configPortalTimeout == 0 || (_apClientCheck && (WiFi_softap_num_stations() > 0)))
    {
        // debug num clients every 30s
        if (millis() - timer > logintvl)
        {
            timer = millis();
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("NUM CLIENTS: "), (String)WiFi_softap_num_stations());
#endif
        }
        _configPortalStart = millis(); // kludge, bump configportal start time to skew timeouts
        return false;
    }

    // handle timeout webclient check
    if (_webClientCheck && (_webPortalAccessed > _configPortalStart) > 0)
        _configPortalStart = _webPortalAccessed;

    // handle timed out
    if (millis() > _configPortalStart + _configPortalTimeout)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("config portal has timed out"));
#endif
        return true; // timeout bail, else do debug logging
    }
    else if (_debug && _debugLevel > 0)
    {
        // log timeout time remaining every 30s
        if ((millis() - timer) > logintvl)
        {
            timer = millis();
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("Portal Timeout In"), (String)((_configPortalStart + _configPortalTimeout - millis()) / 1000) + (String)F(" seconds"));
#endif
        }
    }

    return false;
}

void AsyncWiFiManager::setupDNSD()
{
    dnsServer.reset(new DNSServer());

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
#ifdef AWM_DEBUG_LEVEL
    // DEBUG_AWM("dns server started port: ",DNS_PORT);
    DEBUG_AWM(DEBUG_DEV, F("dns server started with ip: "), WiFi.softAPIP()); // @todo not showing ip
#endif
    dnsServer->start(DNS_PORT, F("*"), WiFi.softAPIP());
}

void AsyncWiFiManager::setupConfigPortal()
{

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Starting Web Portal"));
#endif

    // setup dns and web servers
    server.reset(new AWM_WebServer(_httpPort));

    if (_httpPort != 80)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("http server started with custom port: "), _httpPort); // @todo not showing ip
#endif
    }

    if (_webservercallback != NULL)
    {
        _webservercallback();
    }
    // @todo add a new callback maybe, after webserver started, callback cannot override handlers, but can grab them first

    /* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

    server->on(String(FPSTR(R_root)).c_str(), std::bind(&AsyncWiFiManager::handleRoot, this));
    server->on(String(FPSTR(R_wifi)).c_str(), std::bind(&AsyncWiFiManager::handleWifi, this, true));
    server->on(String(FPSTR(R_wifinoscan)).c_str(), std::bind(&AsyncWiFiManager::handleWifi, this, false));
    server->on(String(FPSTR(R_wifisave)).c_str(), std::bind(&AsyncWiFiManager::handleWifiSave, this));
    server->on(String(FPSTR(R_info)).c_str(), std::bind(&AsyncWiFiManager::handleInfo, this));
    server->on(String(FPSTR(R_param)).c_str(), std::bind(&AsyncWiFiManager::handleParam, this));
    server->on(String(FPSTR(R_paramsave)).c_str(), std::bind(&AsyncWiFiManager::handleParamSave, this));
    server->on(String(FPSTR(R_restart)).c_str(), std::bind(&AsyncWiFiManager::handleReset, this));
    server->on(String(FPSTR(R_exit)).c_str(), std::bind(&AsyncWiFiManager::handleExit, this));
    server->on(String(FPSTR(R_close)).c_str(), std::bind(&AsyncWiFiManager::handleClose, this));
    server->on(String(FPSTR(R_erase)).c_str(), std::bind(&AsyncWiFiManager::handleErase, this, false));
    server->on(String(FPSTR(R_status)).c_str(), std::bind(&AsyncWiFiManager::handleWiFiStatus, this));
    server->onNotFound(std::bind(&AsyncWiFiManager::handleNotFound, this));

    server->on(String(FPSTR(R_update)).c_str(), std::bind(&AsyncWiFiManager::handleUpdate, this));
    server->on(String(FPSTR(R_updatedone)).c_str(), HTTP_POST, std::bind(&AsyncWiFiManager::handleUpdateDone, this), std::bind(&AsyncWiFiManager::handleUpdating, this));

    server->begin(); // Web server start
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("HTTP server started"));
#endif

    if (_preloadwifiscan)
        WiFi_scanNetworks(true, true); // preload wifiscan , async
}

boolean AsyncWiFiManager::startConfigPortal()
{
    String ssid = getDefaultAPName();
    return startConfigPortal(ssid.c_str(), NULL);
}

/**
 * [startConfigPortal description]
 * @access public
 * @param  {[type]} char const         *apName     [description]
 * @param  {[type]} char const         *apPassword [description]
 * @return {[type]}      [description]
 */
boolean AsyncWiFiManager::startConfigPortal(char const *apName, char const *apPassword)
{
    _begin();

    //setup AP
    _apName = apName; // @todo check valid apname ?
    _apPassword = apPassword;

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Starting Config Portal"));
#endif

    if (_apName == "")
        _apName = getDefaultAPName();

    if (!validApPassword())
        return false;

    // HANDLE issues with STA connections, shutdown sta if not connected, or else this will hang channel scanning and softap will not respond
    // @todo sometimes still cannot connect to AP for no known reason, no events in log either
    if (_disableSTA || (!WiFi.isConnected() && _disableSTAConn))
    {
        // this fixes most ap problems, however, simply doing mode(WIFI_AP) does not work if sta connection is hanging, must `wifi_station_disconnect`
        WiFi_Disconnect();
        WiFi_enableSTA(false);
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Disabling STA"));
#endif
    }
    else
    {
        // @todo even if sta is connected, it is possible that softap connections will fail, IOS says "invalid password", windows says "cannot connect to this network" researching
        WiFi_enableSTA(true);
    }

    // init configportal globals to known states
    configPortalActive = true;
    bool result = connect = abort = false; // loop flags, connect true success, abort true break
    uint8_t state;

    _configPortalStart = millis();

// start access point
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Enabling AP"));
#endif
    startAP();
    WiFiSetCountry();

    // do AP callback if set
    if (_apcallback != NULL)
    {
        _apcallback(this);
    }

// init configportal
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("setupConfigPortal"));
#endif
    setupConfigPortal();

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("setupDNSD"));
#endif
    setupDNSD();

    if (!_configPortalIsBlocking)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Config Portal Running, non blocking/processing"));
        if (_configPortalTimeout > 0)
            DEBUG_AWM(DEBUG_VERBOSE, F("Portal Timeout In"), (String)(_configPortalTimeout / 1000) + (String)F(" seconds"));
#endif
        return result;
    }

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Config Portal Running, blocking, waiting for clients..."));
    if (_configPortalTimeout > 0)
        DEBUG_AWM(DEBUG_VERBOSE, F("Portal Timeout In"), (String)(_configPortalTimeout / 1000) + (String)F(" seconds"));
#endif

    // blocking loop waiting for config
    while (1)
    {

        // if timed out or abort, break
        if (configPortalHasTimeout() || abort)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_DEV, F("configportal loop abort"));
#endif
            shutdownConfigPortal();
            result = abort ? portalAbortResult : portalTimeoutResult; // false, false
            break;
        }

        state = processConfigPortal();

        // status change, break
        if (state != WL_IDLE_STATUS)
        {
            result = (state == WL_CONNECTED); // true if connected
            DEBUG_AWM(DEBUG_DEV, F("configportal loop break"));
            break;
        }

        if (!configPortalActive)
            break;

        yield(); // watchdog
    }

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_NOTIFY, F("config portal exiting"));
#endif
    return result;
}

/**
 * [process description]
 * @access public
 * @return {[type]} [description]
 */
boolean AsyncWiFiManager::process()
{
// process mdns, esp32 not required
#if defined(AWM_MDNS) && defined(ESP8266)
    MDNS.update();
#endif

    if (webPortalActive || (configPortalActive && !_configPortalIsBlocking))
    {
        uint8_t state = processConfigPortal();
        return state == WL_CONNECTED;
    }
    return false;
}

//using esp enums returns for now, should be fine
uint8_t AsyncWiFiManager::processConfigPortal()
{
    if (configPortalActive)
    {
        //DNS handler
        dnsServer->processNextRequest();
    }

    //HTTP handler
    server->handleClient();

    // Waiting for save...
    if (connect)
    {
        connect = false;
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("processing save"));
#endif
        if (_enableCaptivePortal)
            delay(_cpclosedelay); // keeps the captiveportal from closing to fast.

        // skip wifi if no ssid
        if (_ssid == "")
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("No ssid, skipping wifi save"));
#endif
        }
        else
        {
            // attempt sta connection to submitted _ssid, _pass
            uint8_t res = connectWifi(_ssid, _pass, _connectonsave) == WL_CONNECTED;
            if (res || (!_connectonsave))
            {
#ifdef AWM_DEBUG_LEVEL
                if (!_connectonsave)
                {
                    DEBUG_AWM(F("SAVED with no connect to new AP"));
                }
                else
                {
                    DEBUG_AWM(F("Connect to new AP [SUCCESS]"));
                    DEBUG_AWM(F("Got IP Address:"));
                    DEBUG_AWM(WiFi.localIP());
                }
#endif

                if (_savewificallback != NULL)
                {
                    _savewificallback();
                }
                shutdownConfigPortal();
                if (!_connectonsave)
                    return WL_IDLE_STATUS;
                return WL_CONNECTED; // CONNECT SUCCESS
            }
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] Connect to new AP Failed"));
#endif
        }

        if (_shouldBreakAfterConfig)
        {

            // do save callback
            // @todo this is more of an exiting callback than a save, clarify when this should actually occur
            // confirm or verify data was saved to make this more accurate callback
            if (_savewificallback != NULL)
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_VERBOSE, F("WiFi/Param save callback"));
#endif
                _savewificallback();
            }
            shutdownConfigPortal();
            return WL_CONNECT_FAILED; // CONNECT FAIL
        }
        else
        {
            // clear save strings
            _ssid = "";
            _pass = "";
            // if connect fails, turn sta off to stabilize AP
            WiFi_Disconnect();
            WiFi_enableSTA(false);
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("Disabling STA"));
#endif
        }
    }

    return WL_IDLE_STATUS;
}

/**
 * [shutdownConfigPortal description]
 * @access public
 * @return bool success (softapdisconnect)
 */
bool AsyncWiFiManager::shutdownConfigPortal()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("shutdownConfigPortal"));
#endif

    if (webPortalActive)
        return false;

    if (configPortalActive)
    {
        //DNS handler
        dnsServer->processNextRequest();
    }

    //HTTP handler
    server->handleClient();

    // @todo what is the proper way to shutdown and free the server up
    server->stop();
    server.reset();

    WiFi.scanDelete(); // free wifi scan results

    if (!configPortalActive)
        return false;

    dnsServer->stop(); //  free heap ?
    dnsServer.reset();

    // turn off AP
    // @todo bug workaround
    // https://github.com/esp8266/Arduino/issues/3793
    // [APdisconnect] set_config failed! *WM: disconnect configportal - softAPdisconnect failed
    // still no way to reproduce reliably

    bool ret = false;
    ret = WiFi.softAPdisconnect(false);

#ifdef AWM_DEBUG_LEVEL
    if (!ret)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] disconnect configportal - softAPdisconnect FAILED"));
    DEBUG_AWM(DEBUG_VERBOSE, F("restoring usermode"), getModeString(_usermode));
#endif
    delay(1000);
    WiFi_Mode(_usermode); // restore users wifi mode, BUG https://github.com/esp8266/Arduino/issues/4372
    if (WiFi.status() == WL_IDLE_STATUS)
    {
        WiFi.reconnect(); // restart wifi since we disconnected it in startconfigportal
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Reconnect, was idle"));
#endif
    }
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("wifi status:"), getWLStatusString(WiFi.status()));
    DEBUG_AWM(DEBUG_VERBOSE, F("wifi mode:"), getModeString(WiFi.getMode()));
#endif
    configPortalActive = false;
    DEBUG_AWM(DEBUG_VERBOSE, F("configportal closed"));
    _end();
    return ret;
}

// @todo refactor this up into seperate functions
// one for connecting to flash , one for new client
// clean up, flow is convoluted, and causes bugs
uint8_t AsyncWiFiManager::connectWifi(String ssid, String pass, bool connect)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("Connecting as wifi client..."));
#endif
    uint8_t retry = 1;
    uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

    setSTAConfig();
    //@todo catch failures in set_config

    // make sure sta is on before `begin` so it does not call enablesta->mode while persistent is ON ( which would save WM AP state to eeprom !)

    if (_cleanConnect)
        WiFi_Disconnect(); // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect
    // @todo find out what status is when this is needed, can we detect it and handle it, say in between states or idle_status

    while (retry <= _connectRetries && (connRes != WL_CONNECTED))
    {
        if (_connectRetries > 1)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("Connect Wifi, ATTEMPT #"), (String)retry + " of " + (String)_connectRetries);
#endif
        }
        // if ssid argument provided connect to that
        if (ssid != "")
        {
            wifiConnectNew(ssid, pass, connect);
            if (_saveTimeout > 0)
            {
                connRes = waitForConnectResult(_saveTimeout); // use default save timeout for saves to prevent bugs in esp->waitforconnectresult loop
            }
            else
            {
                connRes = waitForConnectResult(0);
            }
        }
        else
        {
            // connect using saved ssid if there is one
            if (WiFi_hasAutoConnect())
            {
                wifiConnectDefault();
                connRes = waitForConnectResult();
            }
            else
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(F("No wifi saved, skipping"));
#endif
            }
        }

#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Connection result:"), getWLStatusString(connRes));
#endif
        retry++;
    }

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
    // do WPS, if WPS options enabled and not connected and no password was supplied
    // @todo this seems like wrong place for this, is it a fallback or option?
    if (_tryWPS && connRes != WL_CONNECTED && pass == "")
    {
        startWPS();
        // should be connected at the end of WPS
        connRes = waitForConnectResult();
    }
#endif

    if (connRes != WL_SCAN_COMPLETED)
    {
        updateConxResult(connRes);
    }

    return connRes;
}

/**
 * connect to a new wifi ap
 * @since $dev
 * @param  String ssid 
 * @param  String pass 
 * @return bool success
 * @return connect only save if false
 */
bool AsyncWiFiManager::wifiConnectNew(String ssid, String pass, bool connect)
{
    bool ret = false;
#ifdef AWM_DEBUG_LEVEL
    // DEBUG_AWM(DEBUG_DEV,F("CONNECTED: "),WiFi.status() == WL_CONNECTED ? "Y" : "NO");
    DEBUG_AWM(F("Connecting to NEW AP:"), ssid);
    DEBUG_AWM(DEBUG_DEV, F("Using Password:"), pass);
#endif
    WiFi_enableSTA(true, storeSTAmode); // storeSTAmode will also toggle STA on in default opmode (persistent) if true (default)
    WiFi.persistent(true);
    ret = WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, connect);
    WiFi.persistent(false);
#ifdef AWM_DEBUG_LEVEL
    if (!ret)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] wifi begin failed"));
#endif
    return ret;
}

/**
 * connect to stored wifi
 * @since dev
 * @return bool success
 */
bool AsyncWiFiManager::wifiConnectDefault()
{
    bool ret = false;

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Connecting to SAVED AP:"), WiFi_SSID(true));
    DEBUG_AWM(DEBUG_DEV, F("Using Password:"), WiFi_psk(true));
#endif

    ret = WiFi_enableSTA(true, storeSTAmode);
    delay(500); // THIS DELAY ?

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Mode after delay: "), getModeString(WiFi.getMode()));
    if (!ret)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] wifi enableSta failed"));
#endif

    ret = WiFi.begin();

#ifdef AWM_DEBUG_LEVEL
    if (!ret)
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] wifi begin failed"));
#endif

    return ret;
}

/**
 * set sta config if set
 * @since $dev
 * @return bool success
 */
bool AsyncWiFiManager::setSTAConfig()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("STA static IP:"), _sta_static_ip);
#endif
    bool ret = true;
    if (_sta_static_ip)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Custom static IP/GW/Subnet/DNS"));
#endif
        if (_sta_static_dns)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("Custom static DNS"));
#endif
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
        }
        else
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("Custom STA IP/GW/Subnet"));
#endif
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
        }

#ifdef AWM_DEBUG_LEVEL
        if (!ret)
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] wifi config failed"));
        else
            DEBUG_AWM(F("STA IP set:"), WiFi.localIP());
#endif
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("setSTAConfig static ip not set, skipping"));
#endif
    }
    return ret;
}

// @todo change to getLastFailureReason and do not touch conxresult
void AsyncWiFiManager::updateConxResult(uint8_t status)
{
    // hack in wrong password detection
    _lastconxresult = status;
#ifdef ESP8266
    if (_lastconxresult == WL_CONNECT_FAILED)
    {
        if (wifi_station_get_connect_status() == STATION_WRONG_PASSWORD)
        {
            _lastconxresult = WL_STATION_WRONG_PASSWORD;
        }
    }
#elif defined(ESP32)
    // if(_lastconxresult == WL_CONNECT_FAILED){
    if (_lastconxresult == WL_CONNECT_FAILED || _lastconxresult == WL_DISCONNECTED)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("lastconxresulttmp:"), getWLStatusString(_lastconxresulttmp));
#endif
        if (_lastconxresulttmp != WL_IDLE_STATUS)
        {
            _lastconxresult = _lastconxresulttmp;
            // _lastconxresulttmp = WL_IDLE_STATUS;
        }
    }
    DEBUG_AWM(DEBUG_DEV, F("lastconxresult:"), getWLStatusString(_lastconxresult));
#endif
}

uint8_t AsyncWiFiManager::waitForConnectResult()
{
#ifdef AWM_DEBUG_LEVEL
    if (_connectTimeout > 0)
        DEBUG_AWM(DEBUG_DEV, _connectTimeout, F("ms connectTimeout set"));
#endif
    return waitForConnectResult(_connectTimeout);
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t AsyncWiFiManager::waitForConnectResult(uint32_t timeout)
{
    if (timeout == 0)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("connectTimeout not set, ESP waitForConnectResult..."));
#endif
        return WiFi.waitForConnectResult();
    }

    unsigned long timeoutmillis = millis() + timeout;
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, timeout, F("ms timeout, waiting for connect..."));
#endif
    uint8_t status = WiFi.status();

    while (millis() < timeoutmillis)
    {
        status = WiFi.status();
        // @todo detect additional states, connect happens, then dhcp then get ip, there is some delay here, make sure not to timeout if waiting on IP
        if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
        {
#ifdef AWM_DEBUG_LEVEL
            Serial.println();
#endif
            return status;
        }
#ifdef AWM_DEBUG_LEVEL
        Serial.print(".");
#endif
        delay(100);
    }
    return status;
}

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
void AsyncWiFiManager::startWPS()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("START WPS"));
#endif
#ifdef ESP8266
    WiFi.beginWPSConfig();
#else
    // @todo
#endif
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("END WPS"));
#endif
}
#endif

String AsyncWiFiManager::getHTTPHead(String title)
{
    String page;
    page += FPSTR(HTTP_HEAD_START);
    page.replace(FPSTR(T_v), title);
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;

    if (_bodyClass != "")
    {
        String p = FPSTR(HTTP_HEAD_END);
        p.replace(FPSTR(T_c), _bodyClass); // add class str
        page += p;
    }
    else
    {
        page += FPSTR(HTTP_HEAD_END);
    }

    return page;
}

/** 
 * HTTPD handler for page requests
 */
void AsyncWiFiManager::handleRequest()
{
    _webPortalAccessed = millis();

    // TESTING HTTPD AUTH RFC 2617
    // BASIC_AUTH will hold onto creds, hard to "logout", but convienent
    // DIGEST_AUTH will require new auth often, and nonce is random
    // bool authenticate(const char * username, const char * password);
    // bool authenticateDigest(const String& username, const String& H1);
    // void requestAuthentication(HTTPAuthMethod mode = BASIC_AUTH, const char* realm = NULL, const String& authFailMsg = String("") );

    // 2.3 NO AUTH available
    bool testauth = false;
    if (!testauth)
        return;

    DEBUG_AWM(DEBUG_DEV, F("DOING AUTH"));
    bool res = server->authenticate("admin", "12345");
    if (!res)
    {
#ifndef WM_NOAUTH
        server->requestAuthentication(HTTPAuthMethod::BASIC_AUTH); // DIGEST_AUTH
#endif
        DEBUG_AWM(DEBUG_DEV, F("AUTH FAIL"));
    }
}

/** 
 * HTTPD CALLBACK root or redirect to captive portal
 */
void AsyncWiFiManager::handleRoot()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Root"));
#endif
    if (captivePortal())
        return; // If captive portal redirect instead of displaying the page
    handleRequest();
    String page = getHTTPHead(_title);  // @token options @todo replace options with title
    String str = FPSTR(HTTP_ROOT_MAIN); // @todo custom title
    str.replace(FPSTR(T_t), _title);
    str.replace(FPSTR(T_v), configPortalActive ? _apName : (getWiFiHostname() + " - " + WiFi.localIP().toString())); // use ip if ap is not active for heading @todo use hostname?
    page += str;
    page += FPSTR(HTTP_PORTAL_OPTIONS);
    page += getMenuOut();
    reportStatus(page);
    page += FPSTR(HTTP_END);

    // server->setContentLength(page.length());
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
    // server->close(); // testing reliability fix for content length mismatches during mutiple flood hits  WiFi_scanNetworks(); // preload wifiscan
    if (_preloadwifiscan)
        WiFi_scanNetworks(_scancachetime, true); // preload wifiscan throttled, async
                                                 // @todo buggy, captive portals make a query on every page load, causing this to run every time in addition to the real page load
                                                 // I dont understand why, when you are already in the captive portal, I guess they want to know that its still up and not done or gone
                                                 // if we can detect these and ignore them that would be great, since they come from the captive portal redirect maybe there is a refferer
}

/**
 * HTTPD CALLBACK Wifi config page handler
 */
void AsyncWiFiManager::handleWifi(boolean scan)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Wifi"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titlewifi)); // @token titlewifi
    if (scan)
    {
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(DEBUG_DEV,"refresh flag:",server->hasArg(F("refresh")));
#endif
        WiFi_scanNetworks(server->hasArg(F("refresh")), false); //wifiscan, force if arg refresh
        page += getScanItemOut();
    }
    String pitem = "";

    pitem = FPSTR(HTTP_FORM_START);
    pitem.replace(FPSTR(T_v), F("wifisave")); // set form action
    page += pitem;

    pitem = FPSTR(HTTP_FORM_WIFI);
    pitem.replace(FPSTR(T_v), WiFi_SSID());

    if (_showPassword)
    {
        pitem.replace(FPSTR(T_p), WiFi_psk());
    }
    else if (WiFi_psk() != "")
    {
        pitem.replace(FPSTR(T_p), FPSTR(S_passph));
    }
    else
    {
        pitem.replace(FPSTR(T_p), "");
    }

    page += pitem;

    page += getStaticOut();
    page += FPSTR(HTTP_FORM_WIFI_END);
    if (_paramsInWifi && _paramsCount > 0)
    {
        page += FPSTR(HTTP_FORM_PARAM_HEAD);
        page += getParamOut();
    }
    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_SCAN_LINK);
    if (_showBack)
        page += FPSTR(HTTP_BACKBTN);
    reportStatus(page);
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
    // server->close(); // testing reliability fix for content length mismatches during mutiple flood hits

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Sent config page"));
#endif
}

/**
 * HTTPD CALLBACK Wifi param page handler
 */
void AsyncWiFiManager::handleParam()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Param"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titleparam)); // @token titlewifi

    String pitem = "";

    pitem = FPSTR(HTTP_FORM_START);
    pitem.replace(FPSTR(T_v), F("paramsave"));
    page += pitem;

    page += getParamOut();
    page += FPSTR(HTTP_FORM_END);
    if (_showBack)
        page += FPSTR(HTTP_BACKBTN);
    reportStatus(page);
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Sent param page"));
#endif
}

String AsyncWiFiManager::getMenuOut()
{
    String page;

    for (auto menuId : _menuIds)
    {
        if ((String)_menutokens[menuId] == "param" && _paramsCount == 0)
            continue; // no params set, omit params from menu, @todo this may be undesired by someone, use only menu to force?
        page += HTTP_PORTAL_MENU[menuId];
    }

    return page;
}

// // is it possible in softap mode to detect aps without scanning
// bool AsyncWiFiManager::WiFi_scanNetworksForAP(bool force){
//   WiFi_scanNetworks(force);
// }

void AsyncWiFiManager::WiFi_scanComplete(int networksFound)
{
    _lastscan = millis();
    _numNetworks = networksFound;
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan ASYNC completed"), "in " + (String)(_lastscan - _startscan) + " ms");
    DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan ASYNC found:"), _numNetworks);
#endif
}

bool AsyncWiFiManager::WiFi_scanNetworks()
{
    return WiFi_scanNetworks(false, false);
}

bool AsyncWiFiManager::WiFi_scanNetworks(unsigned int cachetime, bool async)
{
    return WiFi_scanNetworks(millis() - _lastscan > cachetime, async);
}
bool AsyncWiFiManager::WiFi_scanNetworks(unsigned int cachetime)
{
    return WiFi_scanNetworks(millis() - _lastscan > cachetime, false);
}
bool AsyncWiFiManager::WiFi_scanNetworks(bool force, bool async)
{
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(DEBUG_DEV,"scanNetworks async:",async == true);
// DEBUG_AWM(DEBUG_DEV,_numNetworks,(millis()-_lastscan ));
// DEBUG_AWM(DEBUG_DEV,"scanNetworks force:",force == true);
#endif
    if (_numNetworks == 0)
    {
        DEBUG_AWM(DEBUG_DEV, "NO APs found forcing new scan");
        force = true;
    }
    if (force || (millis() - _lastscan > 60000))
    {
        int8_t res;
        _startscan = millis();
        if (async && _asyncScan)
        {
#ifdef ESP8266
#ifndef WM_NOASYNC // no async available < 2.4.0
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan ASYNC started"));
#endif
            using namespace std::placeholders; // for `_1`
            WiFi.scanNetworksAsync(std::bind(&AsyncWiFiManager::WiFi_scanComplete, this, _1));
#else
            DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan SYNC started"));
            res = WiFi.scanNetworks();
#endif
#else
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan ASYNC started"));
#endif
            res = WiFi.scanNetworks(true);
#endif
            return false;
        }
        else
        {
            DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan SYNC started"));
            res = WiFi.scanNetworks();
        }
        if (res == WIFI_SCAN_FAILED)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] scan failed"));
#endif
        }
        else if (res == WIFI_SCAN_RUNNING)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] scan waiting"));
#endif
            while (WiFi.scanComplete() == WIFI_SCAN_RUNNING)
            {
#ifdef AWM_DEBUG_LEVEL
                Serial.print(".");
#endif
                delay(100);
            }
#ifdef AWM_DEBUG_LEVEL
            Serial.println();
#endif
            _numNetworks = WiFi.scanComplete();
        }
        else if (res >= 0)
            _numNetworks = res;
        _lastscan = millis();
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("WiFi Scan completed"), "in " + (String)(_lastscan - _startscan) + " ms");
#endif
        return true;
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Scan is cached"), (String)(millis() - _lastscan) + " ms ago");
#endif
    }
    return false;
}

String AsyncWiFiManager::AsyncWiFiManager::getScanItemOut()
{
    String page;

    if (!_numNetworks)
        WiFi_scanNetworks(); // scan in case this gets called before any scans

    int n = _numNetworks;
    if (n == 0)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("No networks found"));
#endif
        page += FPSTR(S_nonetworks); // @token nonetworks
        page += F("<br/><br/>");
    }
    else
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(n, F("networks found"));
#endif
        //sort networks
        int indices[n];
        for (int i = 0; i < n; i++)
        {
            indices[i] = i;
        }

        // RSSI SORT
        for (int i = 0; i < n; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
                {
                    std::swap(indices[i], indices[j]);
                }
            }
        }

        /* test std:sort
        std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
        {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
        });
       */

        // remove duplicates ( must be RSSI sorted )
        if (_removeDuplicateAPs)
        {
            String cssid;
            for (int i = 0; i < n; i++)
            {
                if (indices[i] == -1)
                    continue;
                cssid = WiFi.SSID(indices[i]);
                for (int j = i + 1; j < n; j++)
                {
                    if (cssid == WiFi.SSID(indices[j]))
                    {
#ifdef AWM_DEBUG_LEVEL
                        DEBUG_AWM(DEBUG_VERBOSE, F("DUP AP:"), WiFi.SSID(indices[j]));
#endif
                        indices[j] = -1; // set dup aps to index -1
                    }
                }
            }
        }

        // token precheck, to speed up replacements on large ap lists
        String HTTP_ITEM_STR = FPSTR(HTTP_ITEM);

        // toggle icons with percentage
        HTTP_ITEM_STR.replace("{qp}", FPSTR(HTTP_ITEM_QP));
        HTTP_ITEM_STR.replace("{h}", _scanDispOptions ? "" : "h");
        HTTP_ITEM_STR.replace("{qi}", FPSTR(HTTP_ITEM_QI));
        HTTP_ITEM_STR.replace("{h}", _scanDispOptions ? "h" : "");

        // set token precheck flags
        bool tok_r = HTTP_ITEM_STR.indexOf(FPSTR(T_r)) > 0;
        bool tok_R = HTTP_ITEM_STR.indexOf(FPSTR(T_R)) > 0;
        bool tok_e = HTTP_ITEM_STR.indexOf(FPSTR(T_e)) > 0;
        bool tok_q = HTTP_ITEM_STR.indexOf(FPSTR(T_q)) > 0;
        bool tok_i = HTTP_ITEM_STR.indexOf(FPSTR(T_i)) > 0;

        //display networks in page
        for (int i = 0; i < n; i++)
        {
            if (indices[i] == -1)
                continue; // skip dups

#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("AP: "), (String)WiFi.RSSI(indices[i]) + " " + (String)WiFi.SSID(indices[i]));
#endif

            int rssiperc = getRSSIasQuality(WiFi.RSSI(indices[i]));
            uint8_t enc_type = WiFi.encryptionType(indices[i]);

            if (_minimumQuality == -1 || _minimumQuality < rssiperc)
            {
                String item = HTTP_ITEM_STR;
                item.replace(FPSTR(T_v), htmlEntities(WiFi.SSID(indices[i]))); // ssid no encoding
                if (tok_e)
                    item.replace(FPSTR(T_e), encryptionTypeStr(enc_type));
                if (tok_r)
                    item.replace(FPSTR(T_r), (String)rssiperc); // rssi percentage 0-100
                if (tok_R)
                    item.replace(FPSTR(T_R), (String)WiFi.RSSI(indices[i])); // rssi db
                if (tok_q)
                    item.replace(FPSTR(T_q), (String) int(round(map(rssiperc, 0, 100, 1, 4)))); //quality icon 1-4
                if (tok_i)
                {
                    if (enc_type != AWM_WIFIOPEN)
                    {
                        item.replace(FPSTR(T_i), F("l"));
                    }
                    else
                    {
                        item.replace(FPSTR(T_i), "");
                    }
                }
#ifdef AWM_DEBUG_LEVEL
//DEBUG_AWM(item);
#endif
                page += item;
                delay(0);
            }
            else
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_VERBOSE, F("Skipping , does not meet _minimumQuality"));
#endif
            }
        }
        page += FPSTR(HTTP_BR);
    }

    return page;
}

String AsyncWiFiManager::getIpForm(String id, String title, String value)
{
    String item = FPSTR(HTTP_FORM_LABEL);
    item += FPSTR(HTTP_FORM_PARAM);
    item.replace(FPSTR(T_i), id);
    item.replace(FPSTR(T_n), id);
    item.replace(FPSTR(T_p), FPSTR(T_t));
    // item.replace(FPSTR(T_p), default);
    item.replace(FPSTR(T_t), title);
    item.replace(FPSTR(T_l), F("15"));
    item.replace(FPSTR(T_v), value);
    item.replace(FPSTR(T_c), "");
    return item;
}

String AsyncWiFiManager::getStaticOut()
{
    String page;
    if ((_staShowStaticFields || _sta_static_ip) && _staShowStaticFields >= 0)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("_staShowStaticFields"));
#endif
        page += FPSTR(HTTP_FORM_STATIC_HEAD);
        // @todo how can we get these accurate settings from memory , wifi_get_ip_info does not seem to reveal if struct ip_info is static or not
        page += getIpForm(FPSTR(S_ip), FPSTR(S_staticip), (_sta_static_ip ? _sta_static_ip.toString() : "")); // @token staticip
        // WiFi.localIP().toString();
        page += getIpForm(FPSTR(S_gw), FPSTR(S_staticgw), (_sta_static_gw ? _sta_static_gw.toString() : "")); // @token staticgw
        // WiFi.gatewayIP().toString();
        page += getIpForm(FPSTR(S_sn), FPSTR(S_subnet), (_sta_static_sn ? _sta_static_sn.toString() : "")); // @token subnet
                                                                                                            // WiFi.subnetMask().toString();
    }

    if ((_staShowDns || _sta_static_dns) && _staShowDns >= 0)
    {
        page += getIpForm(FPSTR(S_dns), FPSTR(S_staticdns), (_sta_static_dns ? _sta_static_dns.toString() : "")); // @token dns
    }

    if (page != "")
        page += FPSTR(HTTP_BR); // @todo remove these, use css

    return page;
}

String AsyncWiFiManager::getParamOut()
{
    String page;

    if (_paramsCount > 0)
    {

        String HTTP_PARAM_temp = FPSTR(HTTP_FORM_LABEL);
        HTTP_PARAM_temp += FPSTR(HTTP_FORM_PARAM);
        bool tok_I = HTTP_PARAM_temp.indexOf(FPSTR(T_I)) > 0;
        bool tok_i = HTTP_PARAM_temp.indexOf(FPSTR(T_i)) > 0;
        bool tok_n = HTTP_PARAM_temp.indexOf(FPSTR(T_n)) > 0;
        bool tok_p = HTTP_PARAM_temp.indexOf(FPSTR(T_p)) > 0;
        bool tok_t = HTTP_PARAM_temp.indexOf(FPSTR(T_t)) > 0;
        bool tok_l = HTTP_PARAM_temp.indexOf(FPSTR(T_l)) > 0;
        bool tok_v = HTTP_PARAM_temp.indexOf(FPSTR(T_v)) > 0;
        bool tok_c = HTTP_PARAM_temp.indexOf(FPSTR(T_c)) > 0;

        char valLength[5];
        // add the extra parameters to the form
        for (int i = 0; i < _paramsCount; i++)
        {
            if (_params[i] == NULL || _params[i]->_length == 0)
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_ERROR, F("[ERROR] AsyncWiFiManagerParameter is out of scope"));
#endif
                break;
            }

            // label before or after, @todo this could be done via floats or CSS and eliminated
            String pitem;
            switch (_params[i]->getLabelPlacement())
            {
            case AWFM_LABEL_BEFORE:
                pitem = FPSTR(HTTP_FORM_LABEL);
                pitem += FPSTR(HTTP_FORM_PARAM);
                break;
            case AWFM_LABEL_AFTER:
                pitem = FPSTR(HTTP_FORM_PARAM);
                pitem += FPSTR(HTTP_FORM_LABEL);
                break;
            default:
                // AWFM_NO_LABEL
                pitem = FPSTR(HTTP_FORM_PARAM);
                break;
            }

            // Input templating
            // "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>";
            // if no ID use customhtml for item, else generate from param string
            if (_params[i]->getID() != NULL)
            {
                if (tok_I)
                    pitem.replace(FPSTR(T_I), (String)FPSTR(S_parampre) + (String)i); // T_I id number
                if (tok_i)
                    pitem.replace(FPSTR(T_i), _params[i]->getID()); // T_i id name
                if (tok_n)
                    pitem.replace(FPSTR(T_n), _params[i]->getID()); // T_n id name alias
                if (tok_p)
                    pitem.replace(FPSTR(T_p), FPSTR(T_t)); // T_p replace legacy placeholder token
                if (tok_t)
                    pitem.replace(FPSTR(T_t), _params[i]->getLabel()); // T_t title/label
                snprintf(valLength, 5, "%d", _params[i]->getValueLength());
                if (tok_l)
                    pitem.replace(FPSTR(T_l), valLength); // T_l value length
                if (tok_v)
                    pitem.replace(FPSTR(T_v), _params[i]->getValue()); // T_v value
                if (tok_c)
                    pitem.replace(FPSTR(T_c), _params[i]->getCustomHTML()); // T_c meant for additional attributes, not html, but can stuff
            }
            else
            {
                pitem = _params[i]->getCustomHTML();
            }

            page += pitem;
        }
    }

    return page;
}

void AsyncWiFiManager::handleWiFiStatus()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP WiFi status "));
#endif
    handleRequest();
    String page;
// String page = "{\"result\":true,\"count\":1}";
#ifdef AWM_JSTEST
    page = FPSTR(HTTP_JS);
#endif
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
}

/** 
 * HTTPD CALLBACK save form and redirect to WLAN config page again
 */
void AsyncWiFiManager::handleWifiSave()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP WiFi save "));
    DEBUG_AWM(DEBUG_DEV, F("Method:"), server->method() == HTTP_GET ? (String)FPSTR(S_GET) : (String)FPSTR(S_POST));
#endif
    handleRequest();

    // @todo use new callback for before paramsaves
    if (_presavecallback != NULL)
    {
        _presavecallback();
    }

    //SAVE/connect here
    _ssid = server->arg(F("s")).c_str();
    _pass = server->arg(F("p")).c_str();

    if (_paramsInWifi)
        doParamSave();

    if (server->arg(FPSTR(S_ip)) != "")
    {
        //_sta_static_ip.fromString(server->arg(FPSTR(S_ip));
        String ip = server->arg(FPSTR(S_ip));
        optionalIPFromString(&_sta_static_ip, ip.c_str());
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("static ip:"), ip);
#endif
    }
    if (server->arg(FPSTR(S_gw)) != "")
    {
        String gw = server->arg(FPSTR(S_gw));
        optionalIPFromString(&_sta_static_gw, gw.c_str());
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("static gateway:"), gw);
#endif
    }
    if (server->arg(FPSTR(S_sn)) != "")
    {
        String sn = server->arg(FPSTR(S_sn));
        optionalIPFromString(&_sta_static_sn, sn.c_str());
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("static netmask:"), sn);
#endif
    }
    if (server->arg(FPSTR(S_dns)) != "")
    {
        String dns = server->arg(FPSTR(S_dns));
        optionalIPFromString(&_sta_static_dns, dns.c_str());
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("static DNS:"), dns);
#endif
    }

    String page;

    if (_ssid == "")
    {
        page = getHTTPHead(FPSTR(S_titlewifisettings)); // @token titleparamsaved
        page += FPSTR(HTTP_PARAMSAVED);
    }
    else
    {
        page = getHTTPHead(FPSTR(S_titlewifisaved)); // @token titlewifisaved
        page += FPSTR(HTTP_SAVED);
    }
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->sendHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Sent wifi save page"));
#endif

    connect = true; //signal ready to connect/reset process in processConfigPortal
}

void AsyncWiFiManager::handleParamSave()
{

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Param save "));
#endif
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Method:"), server->method() == HTTP_GET ? (String)FPSTR(S_GET) : (String)FPSTR(S_POST));
#endif
    handleRequest();

    doParamSave();

    String page = getHTTPHead(FPSTR(S_titleparamsaved)); // @token titleparamsaved
    page += FPSTR(HTTP_PARAMSAVED);
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Sent param save page"));
#endif
}

void AsyncWiFiManager::doParamSave()
{
    // @todo use new callback for before paramsaves, is this really needed?
    if (_presavecallback != NULL)
    {
        _presavecallback();
    }

    //parameters
    if (_paramsCount > 0)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Parameters"));
        DEBUG_AWM(DEBUG_VERBOSE, FPSTR(D_HR));
#endif

        for (int i = 0; i < _paramsCount; i++)
        {
            if (_params[i] == NULL || _params[i]->_length == 0)
            {
#ifdef AWM_DEBUG_LEVEL
                DEBUG_AWM(DEBUG_ERROR, F("[ERROR] AsyncWiFiManagerParameter is out of scope"));
#endif
                break; // @todo might not be needed anymore
            }
            //read parameter from server
            String name = (String)FPSTR(S_parampre) + (String)i;
            String value;
            if (server->hasArg(name))
            {
                value = server->arg(name);
            }
            else
            {
                value = server->arg(_params[i]->getID());
            }

            //store it in params array
            value.toCharArray(_params[i]->_value, _params[i]->_length + 1); // length+1 null terminated
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, (String)_params[i]->getID() + ":", value);
#endif
        }
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, FPSTR(D_HR));
#endif
    }

    if (_saveparamscallback != NULL)
    {
        _saveparamscallback();
    }
}

/** 
 * HTTPD CALLBACK info page
 */
void AsyncWiFiManager::handleInfo()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Info"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titleinfo)); // @token titleinfo
    reportStatus(page);

    uint16_t infos = 0;

//@todo convert to enum or refactor to strings
//@todo wrap in build flag to remove all info code for memory saving
#ifdef ESP8266
    infos = 29;
    String infoids[] = {
        F("esphead"),
        F("uptime"),
        F("chipid"),
        F("fchipid"),
        F("idesize"),
        F("flashsize"),
        F("sdkver"),
        F("corever"),
        F("bootver"),
        F("cpufreq"),
        F("freeheap"),
        F("memsketch"),
        F("memsmeter"),
        F("lastreset"),
        F("wifihead"),
        F("conx"),
        F("stassid"),
        F("staip"),
        F("stagw"),
        F("stasub"),
        F("dnss"),
        F("host"),
        F("stamac"),
        F("autoconx"),
        F("wifiaphead"),
        F("apssid"),
        F("apip"),
        F("apbssid"),
        F("apmac")};

#elif defined(ESP32)
    // add esp_chip_info ?
    infos = 27;
    String infoids[] = {
        F("esphead"),
        F("uptime"),
        F("chipid"),
        F("chiprev"),
        F("idesize"),
        F("flashsize"),
        F("sdkver"),
        F("cpufreq"),
        F("freeheap"),
        F("memsketch"),
        F("memsmeter"),
        F("lastreset"),
        F("wifihead"),
        F("conx"),
        F("stassid"),
        F("staip"),
        F("stagw"),
        F("stasub"),
        F("dnss"),
        F("host"),
        F("stamac"),
        F("apssid"),
        F("wifiaphead"),
        F("apip"),
        F("apmac"),
        F("aphost"),
        F("apbssid")
        // F("temp")
    };
#endif

    for (size_t i = 0; i < infos; i++)
    {
        if (infoids[i] != NULL)
            page += getInfoData(infoids[i]);
    }
    page += F("</dl>");
    if (_showInfoUpdate)
    {
        page += HTTP_PORTAL_MENU[8];
        page += HTTP_PORTAL_MENU[9];
    }
    if (_showInfoErase)
        page += FPSTR(HTTP_ERASEBTN);
    if (_showBack)
        page += FPSTR(HTTP_BACKBTN);
    page += FPSTR(HTTP_HELP);
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("Sent info page"));
#endif
}

String AsyncWiFiManager::getInfoData(String id)
{

    String p;
    // @todo add WM versioning
    if (id == F("esphead"))
        p = FPSTR(HTTP_INFO_esphead);
    else if (id == F("wifihead"))
        p = FPSTR(HTTP_INFO_wifihead);
    else if (id == F("uptime"))
    {
        // subject to rollover!
        p = FPSTR(HTTP_INFO_uptime);
        p.replace(FPSTR(T_1), (String)(millis() / 1000 / 60));
        p.replace(FPSTR(T_2), (String)((millis() / 1000) % 60));
    }
    else if (id == F("chipid"))
    {
        p = FPSTR(HTTP_INFO_chipid);
        p.replace(FPSTR(T_1), String(WIFI_getChipId(), HEX));
    }
#ifdef ESP32
    else if (id == F("chiprev"))
    {
        p = FPSTR(HTTP_INFO_chiprev);
        String rev = (String)ESP.getChipRevision();
#ifdef _SOC_EFUSE_REG_H_
        String revb = (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S) && EFUSE_RD_CHIP_VER_RESERVE_V);
        p.replace(FPSTR(T_1), rev + "<br/>" + revb);
#else
        p.replace(FPSTR(T_1), rev);
#endif
    }
#endif
#ifdef ESP8266
    else if (id == F("fchipid"))
    {
        p = FPSTR(HTTP_INFO_fchipid);
        p.replace(FPSTR(T_1), (String)ESP.getFlashChipId());
    }
#endif
    else if (id == F("idesize"))
    {
        p = FPSTR(HTTP_INFO_idesize);
        p.replace(FPSTR(T_1), (String)ESP.getFlashChipSize());
    }
    else if (id == F("flashsize"))
    {
#ifdef ESP8266
        p = FPSTR(HTTP_INFO_flashsize);
        p.replace(FPSTR(T_1), (String)ESP.getFlashChipRealSize());
#elif defined ESP32
        p = FPSTR(HTTP_INFO_psrsize);
        p.replace(FPSTR(T_1), (String)ESP.getPsramSize());
#endif
    }
    else if (id == F("sdkver"))
    {
        p = FPSTR(HTTP_INFO_sdkver);
#ifdef ESP32
        p.replace(FPSTR(T_1), (String)esp_get_idf_version());
        // p.replace(FPSTR(T_1),(String)system_get_sdk_version()); // deprecated
#else
        p.replace(FPSTR(T_1), (String)system_get_sdk_version());
#endif
    }
    else if (id == F("corever"))
    {
#ifdef ESP8266
        p = FPSTR(HTTP_INFO_corever);
        p.replace(FPSTR(T_1), (String)ESP.getCoreVersion());
#endif
    }
#ifdef ESP8266
    else if (id == F("bootver"))
    {
        p = FPSTR(HTTP_INFO_bootver);
        p.replace(FPSTR(T_1), (String)system_get_boot_version());
    }
#endif
    else if (id == F("cpufreq"))
    {
        p = FPSTR(HTTP_INFO_cpufreq);
        p.replace(FPSTR(T_1), (String)ESP.getCpuFreqMHz());
    }
    else if (id == F("freeheap"))
    {
        p = FPSTR(HTTP_INFO_freeheap);
        p.replace(FPSTR(T_1), (String)ESP.getFreeHeap());
    }
    else if (id == F("memsketch"))
    {
        p = FPSTR(HTTP_INFO_memsketch);
        p.replace(FPSTR(T_1), (String)(ESP.getSketchSize()));
        p.replace(FPSTR(T_2), (String)(ESP.getSketchSize() + ESP.getFreeSketchSpace()));
    }
    else if (id == F("memsmeter"))
    {
        p = FPSTR(HTTP_INFO_memsmeter);
        p.replace(FPSTR(T_1), (String)(ESP.getSketchSize()));
        p.replace(FPSTR(T_2), (String)(ESP.getSketchSize() + ESP.getFreeSketchSpace()));
    }
    else if (id == F("lastreset"))
    {
#ifdef ESP8266
        p = FPSTR(HTTP_INFO_lastreset);
        p.replace(FPSTR(T_1), (String)ESP.getResetReason());
#elif defined(ESP32) && defined(_ROM_RTC_H_)
        // requires #include <rom/rtc.h>
        p = FPSTR(HTTP_INFO_lastreset);
        for (int i = 0; i < 2; i++)
        {
            int reason = rtc_get_reset_reason(i);
            String tok = (String)T_ss + (String)(i + 1) + (String)T_es;
            switch (reason)
            {
            //@todo move to array
            case 1:
                p.replace(tok, F("Vbat power on reset"));
                break;
            case 3:
                p.replace(tok, F("Software reset digital core"));
                break;
            case 4:
                p.replace(tok, F("Legacy watch dog reset digital core"));
                break;
            case 5:
                p.replace(tok, F("Deep Sleep reset digital core"));
                break;
            case 6:
                p.replace(tok, F("Reset by SLC module, reset digital core"));
                break;
            case 7:
                p.replace(tok, F("Timer Group0 Watch dog reset digital core"));
                break;
            case 8:
                p.replace(tok, F("Timer Group1 Watch dog reset digital core"));
                break;
            case 9:
                p.replace(tok, F("RTC Watch dog Reset digital core"));
                break;
            case 10:
                p.replace(tok, F("Instrusion tested to reset CPU"));
                break;
            case 11:
                p.replace(tok, F("Time Group reset CPU"));
                break;
            case 12:
                p.replace(tok, F("Software reset CPU"));
                break;
            case 13:
                p.replace(tok, F("RTC Watch dog Reset CPU"));
                break;
            case 14:
                p.replace(tok, F("for APP CPU, reseted by PRO CPU"));
                break;
            case 15:
                p.replace(tok, F("Reset when the vdd voltage is not stable"));
                break;
            case 16:
                p.replace(tok, F("RTC Watch dog reset digital core and rtc module"));
                break;
            default:
                p.replace(tok, F("NO_MEAN"));
            }
        }
#endif
    }
    else if (id == F("apip"))
    {
        p = FPSTR(HTTP_INFO_apip);
        p.replace(FPSTR(T_1), WiFi.softAPIP().toString());
    }
    else if (id == F("apmac"))
    {
        p = FPSTR(HTTP_INFO_apmac);
        p.replace(FPSTR(T_1), (String)WiFi.softAPmacAddress());
    }
#ifdef ESP32
    else if (id == F("aphost"))
    {
        p = FPSTR(HTTP_INFO_aphost);
        p.replace(FPSTR(T_1), WiFi.softAPgetHostname());
    }
#endif
#ifdef ESP8266
#ifndef WM_NOSOFTAPSSID
    else if (id == F("apssid"))
    {
        p = FPSTR(HTTP_INFO_apssid);
        p.replace(FPSTR(T_1), htmlEntities(WiFi.softAPSSID()));
    }
#endif
#endif
    else if (id == F("apbssid"))
    {
        p = FPSTR(HTTP_INFO_apbssid);
        p.replace(FPSTR(T_1), (String)WiFi.BSSIDstr());
    }
    // softAPgetHostname // esp32
    // softAPSubnetCIDR
    // softAPNetworkID
    // softAPBroadcastIP

    else if (id == F("stassid"))
    {
        p = FPSTR(HTTP_INFO_stassid);
        p.replace(FPSTR(T_1), htmlEntities((String)WiFi_SSID()));
    }
    else if (id == F("staip"))
    {
        p = FPSTR(HTTP_INFO_staip);
        p.replace(FPSTR(T_1), WiFi.localIP().toString());
    }
    else if (id == F("stagw"))
    {
        p = FPSTR(HTTP_INFO_stagw);
        p.replace(FPSTR(T_1), WiFi.gatewayIP().toString());
    }
    else if (id == F("stasub"))
    {
        p = FPSTR(HTTP_INFO_stasub);
        p.replace(FPSTR(T_1), WiFi.subnetMask().toString());
    }
    else if (id == F("dnss"))
    {
        p = FPSTR(HTTP_INFO_dnss);
        p.replace(FPSTR(T_1), WiFi.dnsIP().toString());
    }
    else if (id == F("host"))
    {
        p = FPSTR(HTTP_INFO_host);
#ifdef ESP32
        p.replace(FPSTR(T_1), WiFi.getHostname());
#else
        p.replace(FPSTR(T_1), WiFi.hostname());
#endif
    }
    else if (id == F("stamac"))
    {
        p = FPSTR(HTTP_INFO_stamac);
        p.replace(FPSTR(T_1), WiFi.macAddress());
    }
    else if (id == F("conx"))
    {
        p = FPSTR(HTTP_INFO_conx);
        p.replace(FPSTR(T_1), WiFi.isConnected() ? FPSTR(S_y) : FPSTR(S_n));
    }
#ifdef ESP8266
    else if (id == F("autoconx"))
    {
        p = FPSTR(HTTP_INFO_autoconx);
        p.replace(FPSTR(T_1), WiFi.getAutoConnect() ? FPSTR(S_enable) : FPSTR(S_disable));
    }
#endif
#ifdef ESP32
    else if (id == F("temp"))
    {
        // temperature is not calibrated, varying large offsets are present, use for relative temp changes only
        p = FPSTR(HTTP_INFO_temp);
        p.replace(FPSTR(T_1), (String)temperatureRead());
        p.replace(FPSTR(T_2), (String)((temperatureRead() + 32) * 1.8));
        p.replace(FPSTR(T_3), (String)hallRead());
    }
#endif
    return p;
}

/** 
 * HTTPD CALLBACK exit, closes configportal if blocking, if non blocking undefined
 */
void AsyncWiFiManager::handleExit()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Exit"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titleexit)); // @token titleexit
    page += FPSTR(S_exiting);                      // @token exiting
    server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    // ('Logout', 401, {'WWW-Authenticate': 'Basic realm="Login required"'})
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
    delay(2000);
    abort = true;
}

/** 
 * HTTPD CALLBACK reset page
 */
void AsyncWiFiManager::handleReset()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP Reset"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titlereset)); //@token titlereset
    page += FPSTR(S_resetting);                     //@token resetting
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("RESETTING ESP"));
#endif
    delay(1000);
    reboot();
}

/** 
 * HTTPD CALLBACK erase page
 */

// void AsyncWiFiManager::handleErase() {
//   handleErase(false);
// }
void AsyncWiFiManager::handleErase(boolean opt)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_NOTIFY, F("<- HTTP Erase"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titleerase)); // @token titleerase

    bool ret = erase(opt);

    if (ret)
        page += FPSTR(S_resetting); // @token resetting
    else
    {
        page += FPSTR(S_error); // @token erroroccur
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] WiFi EraseConfig failed"));
#endif
    }

    page += FPSTR(HTTP_END);
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

    if (ret)
    {
        delay(2000);
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("RESETTING ESP"));
#endif
        reboot();
    }
}

/** 
 * HTTPD CALLBACK 404
 */
void AsyncWiFiManager::handleNotFound()
{
    if (captivePortal())
        return; // If captive portal redirect instead of displaying the page
    handleRequest();
    String message = FPSTR(S_notfound); // @token notfound
    message += FPSTR(S_uri);            // @token uri
    message += server->uri();
    message += FPSTR(S_method); // @token method
    message += (server->method() == HTTP_GET) ? FPSTR(S_GET) : FPSTR(S_POST);
    message += FPSTR(S_args); // @token args
    message += server->args();
    message += F("\n");

    for (uint8_t i = 0; i < server->args(); i++)
    {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    server->sendHeader(F("Pragma"), F("no-cache"));
    server->sendHeader(F("Expires"), F("-1"));
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(message.length()));
    server->send(404, FPSTR(HTTP_HEAD_CT2), message);
}

/**
 * HTTPD redirector
 * Redirect to captive portal if we got a request for another domain. 
 * Return true in that case so the page handler do not try to handle the request again. 
 */
boolean AsyncWiFiManager::captivePortal()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, "-> " + server->hostHeader());
#endif

    if (!_enableCaptivePortal)
        return false; // skip redirections, @todo maybe allow redirection even when no cp ? might be useful

    String serverLoc = toStringIp(server->client().localIP());
    if (_httpPort != 80)
        serverLoc += ":" + (String)_httpPort;            // add port if not default
    bool doredirect = serverLoc != server->hostHeader(); // redirect if hostheader not server ip, prevent redirect loops
    // doredirect = !isIp(server->hostHeader()) // old check

    if (doredirect)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("<- Request redirected to captive portal"));
#endif
        server->sendHeader(F("Location"), (String)F("http://") + serverLoc, true);
        server->send(302, FPSTR(HTTP_HEAD_CT2), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        server->client().stop();                     // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

void AsyncWiFiManager::stopCaptivePortal()
{
    _enableCaptivePortal = false;
    // @todo maybe disable configportaltimeout(optional), or just provide callback for user
}

// HTTPD CALLBACK, handle close,  stop captive portal, if not enabled undefined
void AsyncWiFiManager::handleClose()
{
    DEBUG_AWM(DEBUG_VERBOSE, F("Disabling Captive Portal"));
    stopCaptivePortal();
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- HTTP close"));
#endif
    handleRequest();
    String page = getHTTPHead(FPSTR(S_titleclose)); // @token titleclose
    page += FPSTR(S_closing);                       // @token closing
    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
}

void AsyncWiFiManager::reportStatus(String &page)
{
    // updateConxResult(WiFi.status()); // @todo: this defeats the purpose of last result, update elsewhere or add logic here
    DEBUG_AWM(DEBUG_DEV, F("[WIFI] reportStatus prev:"), getWLStatusString(_lastconxresult));
    DEBUG_AWM(DEBUG_DEV, F("[WIFI] reportStatus current:"), getWLStatusString(WiFi.status()));
    String str;
    if (WiFi_SSID() != "")
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            str = FPSTR(HTTP_STATUS_ON);
            str.replace(FPSTR(T_i), WiFi.localIP().toString());
            str.replace(FPSTR(T_v), htmlEntities(WiFi_SSID()));
        }
        else
        {
            str = FPSTR(HTTP_STATUS_OFF);
            str.replace(FPSTR(T_v), htmlEntities(WiFi_SSID()));
            if (_lastconxresult == WL_STATION_WRONG_PASSWORD)
            {
                // wrong password
                str.replace(FPSTR(T_c), "D"); // class
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFPW));
            }
            else if (_lastconxresult == WL_NO_SSID_AVAIL)
            {
                // connect failed, or ap not found
                str.replace(FPSTR(T_c), "D");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFNOAP));
            }
            else if (_lastconxresult == WL_CONNECT_FAILED)
            {
                // connect failed
                str.replace(FPSTR(T_c), "D");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFFAIL));
            }
            else
            {
                str.replace(FPSTR(T_c), "");
                str.replace(FPSTR(T_r), "");
            }
        }
    }
    else
    {
        str = FPSTR(HTTP_STATUS_NONE);
    }
    page += str;
}

// PUBLIC

// METHODS

/**
 * reset wifi settings, clean stored ap password
 */

/**
 * [stopConfigPortal description]
 * @return {[type]} [description]
 */
bool AsyncWiFiManager::stopConfigPortal()
{
    if (_configPortalIsBlocking)
    {
        abort = true;
        return true;
    }
    return shutdownConfigPortal();
}

/**
 * disconnect
 * @access public
 * @since $dev
 * @return bool success
 */
bool AsyncWiFiManager::disconnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("Disconnecting: Not connected"));
#endif
        return false;
    }
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Disconnecting"));
#endif
    return WiFi_Disconnect();
}

/**
 * reboot the device
 * @access public
 */
void AsyncWiFiManager::reboot()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Restarting"));
#endif
    ESP.restart();
}

/**
 * reboot the device
 * @access public
 */
bool AsyncWiFiManager::erase()
{
    return erase(false);
}

bool AsyncWiFiManager::erase(bool opt)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM("Erasing");
#endif

#if defined(ESP32) && ((defined(AWM_ERASE_NVS) || defined(nvs_flash_h)))
    // if opt true, do nvs erase
    if (opt)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("Erasing NVS"));
#endif
        esp_err_t err;
        err = nvs_flash_init();
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("nvs_flash_init: "), err != ESP_OK ? (String)err : "Success");
#endif
        err = nvs_flash_erase();
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("nvs_flash_erase: "), err != ESP_OK ? (String)err : "Success");
#endif
        return err == ESP_OK;
    }
#elif defined(ESP8266) && defined(spiffs_api_h)
    if (opt)
    {
        bool ret = false;
        if (SPIFFS.begin())
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("Erasing SPIFFS"));
#endif
            bool ret = SPIFFS.format();
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("spiffs erase: "), ret ? "Success" : "ERROR");
#endif
        }
        else
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("[ERROR] Could not start SPIFFS"));
#endif
        }
        return ret;
    }
#else
    (void)opt;
#endif

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Erasing WiFi Config"));
#endif
    return WiFi_eraseConfig();
}

/**
 * [resetSettings description]
 * ERASES STA CREDENTIALS
 * @access public
 */
void AsyncWiFiManager::resetSettings()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("resetSettings"));
#endif
    WiFi_enableSTA(true, true); // must be sta to disconnect erase

    if (_resetcallback != NULL)
        _resetcallback();

#ifdef ESP32
    WiFi.disconnect(true, true);
#else
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
#endif
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("SETTINGS ERASED"));
#endif
}

// SETTERS

/**
 * [setTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void AsyncWiFiManager::setTimeout(unsigned long seconds)
{
    setConfigPortalTimeout(seconds);
}

/**
 * [setConfigPortalTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void AsyncWiFiManager::setConfigPortalTimeout(unsigned long seconds)
{
    _configPortalTimeout = seconds * 1000;
}

/**
 * [setConnectTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void AsyncWiFiManager::setConnectTimeout(unsigned long seconds)
{
    _connectTimeout = seconds * 1000;
}

/**
 * [setConnectRetries description]
 * @access public
 * @param {[type]} uint8_t numRetries [description]
 */
void AsyncWiFiManager::setConnectRetries(uint8_t numRetries)
{
    _connectRetries = constrain(numRetries, 1, 10);
}

/**
 * toggle _cleanconnect, always disconnect before connecting
 * @param {[type]} bool enable [description]
 */
void AsyncWiFiManager::setCleanConnect(bool enable)
{
    _cleanConnect = enable;
}

/**
 * [setConnectTimeout description
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void AsyncWiFiManager::setSaveConnectTimeout(unsigned long seconds)
{
    _saveTimeout = seconds * 1000;
}

/**
 * Set save portal connect on save option, 
 * if false, will only save credentials not connect
 * @access public
 * @param {[type]} bool connect [description]
 */
void AsyncWiFiManager::setSaveConnect(bool connect)
{
    _connectonsave = connect;
}

/**
 * [setDebugOutput description]
 * @access public
 * @param {[type]} boolean debug [description]
 */
void AsyncWiFiManager::setDebugOutput(boolean debug)
{
    _debug = debug;
    if (_debug && _debugLevel == DEBUG_DEV)
        debugPlatformInfo();
}

void AsyncWiFiManager::setDebugOutput(boolean debug, String prefix)
{
    _debugPrefix = prefix;
    setDebugOutput(debug);
}

/**
 * [setAPStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void AsyncWiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _ap_static_ip = ip;
    _ap_static_gw = gw;
    _ap_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void AsyncWiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _sta_static_ip = ip;
    _sta_static_gw = gw;
    _sta_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @since $dev
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 * @param {[type]} IPAddress dns [description]
 */
void AsyncWiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns)
{
    setSTAStaticIPConfig(ip, gw, sn);
    _sta_static_dns = dns;
}

/**
 * [setMinimumSignalQuality description]
 * @access public
 * @param {[type]} int quality [description]
 */
void AsyncWiFiManager::setMinimumSignalQuality(int quality)
{
    _minimumQuality = quality;
}

/**
 * [setBreakAfterConfig description]
 * @access public
 * @param {[type]} boolean shouldBreak [description]
 */
void AsyncWiFiManager::setBreakAfterConfig(boolean shouldBreak)
{
    _shouldBreakAfterConfig = shouldBreak;
}

/**
 * setAPCallback, set a callback when softap is started
 * @access public 
 * @param {[type]} void (*func)(AsyncWiFiManager* wminstance)
 */
void AsyncWiFiManager::setAPCallback(std::function<void(AsyncWiFiManager *)> func)
{
    _apcallback = func;
}

/**
 * setWebServerCallback, set a callback after webserver is reset, and before routes are setup
 * if we set webserver handlers before wm, they are used and wm is not by esp webserver
 * on events cannot be overrided once set, and are not mutiples
 * @access public 
 * @param {[type]} void (*func)(void)
 */
void AsyncWiFiManager::setWebServerCallback(std::function<void()> func)
{
    _webservercallback = func;
}

/**
 * setSaveConfigCallback, set a save config callback after closing configportal
 * @note calls only if wifi is saved or changed, or setBreakAfterConfig(true)
 * @access public
 * @param {[type]} void (*func)(void)
 */
void AsyncWiFiManager::setSaveConfigCallback(std::function<void()> func)
{
    _savewificallback = func;
}

/**
 * setConfigResetCallback, set a callback to occur when a resetSettings() occurs
 * @access public
 * @param {[type]} void(*func)(void)
 */
void AsyncWiFiManager::setConfigResetCallback(std::function<void()> func)
{
    _resetcallback = func;
}

/**
 * setSaveParamsCallback, set a save params callback on params save in wifi or params pages
 * @access public
 * @param {[type]} void (*func)(void)
 */
void AsyncWiFiManager::setSaveParamsCallback(std::function<void()> func)
{
    _saveparamscallback = func;
}

/**
 * setPreSaveConfigCallback, set a callback to fire before saving wifi or params
 * @access public
 * @param {[type]} void (*func)(void)
 */
void AsyncWiFiManager::setPreSaveConfigCallback(std::function<void()> func)
{
    _presavecallback = func;
}

/**
 * set custom head html
 * custom element will be added to head, eg. new style tag etc.
 * @access public
 * @param char element
 */
void AsyncWiFiManager::setCustomHeadElement(const char *element)
{
    _customHeadElement = element;
}

/**
 * toggle wifiscan hiding of duplicate ssid names
 * if this is false, wifiscan will remove duplicat Access Points - defaut true
 * @access public
 * @param boolean removeDuplicates [true]
 */
void AsyncWiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates)
{
    _removeDuplicateAPs = removeDuplicates;
}

/**
 * toggle configportal blocking loop
 * if enabled, then the configportal will enter a blocking loop and wait for configuration
 * if disabled use with process() to manually process webserver
 * @since $dev
 * @access public
 * @param boolean shoudlBlock [false]
 */
void AsyncWiFiManager::setConfigPortalBlocking(boolean shoudlBlock)
{
    _configPortalIsBlocking = shoudlBlock;
}

/**
 * toggle restore persistent, track internally
 * sets ESP wifi.persistent so we can remember it and restore user preference on destruct
 * there is no getter in esp8266 platform prior to https://github.com/esp8266/Arduino/pull/3857
 * @since $dev
 * @access public
 * @param boolean persistent [true]
 */
void AsyncWiFiManager::setRestorePersistent(boolean persistent)
{
    _userpersistent = persistent;
    if (!persistent)
    {
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(F("persistent is off"));
#endif
    }
}

/**
 * toggle showing static ip form fields
 * if enabled, then the static ip, gateway, subnet fields will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void AsyncWiFiManager::setShowStaticFields(boolean alwaysShow)
{
    if (_disableIpFields)
        _staShowStaticFields = alwaysShow ? 1 : -1;
    else
        _staShowStaticFields = alwaysShow ? 1 : 0;
}

/**
 * toggle showing dns fields
 * if enabled, then the dns1 field will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void AsyncWiFiManager::setShowDnsFields(boolean alwaysShow)
{
    if (_disableIpFields)
        _staShowDns = alwaysShow ? 1 : -1;
    _staShowDns = alwaysShow ? 1 : 0;
}

/**
 * toggle showing password in wifi password field
 * if not enabled, placeholder will be S_passph
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void AsyncWiFiManager::setShowPassword(boolean show)
{
    _showPassword = show;
}

/**
 * toggle captive portal
 * if enabled, then devices that use captive portal checks will be redirected to root
 * if not you will automatically have to navigate to ip [192.168.4.1]
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void AsyncWiFiManager::setCaptivePortalEnable(boolean enabled)
{
    _enableCaptivePortal = enabled;
}

/**
 * toggle wifi autoreconnect policy
 * if enabled, then wifi will autoreconnect automatically always
 * On esp8266 we force this on when autoconnect is called, see notes
 * On esp32 this is handled on SYSTEM_EVENT_STA_DISCONNECTED since it does not exist in core yet
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void AsyncWiFiManager::setWiFiAutoReconnect(boolean enabled)
{
    _wifiAutoReconnect = enabled;
}

/**
 * toggle configportal timeout wait for station client
 * if enabled, then the configportal will start timeout when no stations are connected to softAP
 * disabled by default as rogue stations can keep it open if there is no auth
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void AsyncWiFiManager::setAPClientCheck(boolean enabled)
{
    _apClientCheck = enabled;
}

/**
 * toggle configportal timeout wait for web client
 * if enabled, then the configportal will restart timeout when client requests come in
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void AsyncWiFiManager::setWebPortalClientCheck(boolean enabled)
{
    _webClientCheck = enabled;
}

/**
 * toggle wifiscan percentages or quality icons
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void AsyncWiFiManager::setScanDispPerc(boolean enabled)
{
    _scanDispOptions = enabled;
}

/**
 * toggle configportal if autoconnect failed
 * if enabled, then the configportal will be activated on autoconnect failure
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void AsyncWiFiManager::setEnableConfigPortal(boolean enable)
{
    _enableConfigPortal = enable;
}

/**
 * set the hostname (dhcp client id)
 * @since $dev
 * @access public
 * @param  char* hostname 32 character hostname to use for sta+ap in esp32, sta in esp8266
 * @return bool false if hostname is not valid
 */
bool AsyncWiFiManager::setHostname(const char *hostname)
{
    //@todo max length 32
    _hostname = hostname;
    return true;
}

/**
 * set the soft ao channel, ignored if channelsync is true and connected
 * @param int32_t   wifi channel, 0 to disable
 */
void AsyncWiFiManager::setWiFiAPChannel(int32_t channel)
{
    _apChannel = channel;
}

/**
 * set the soft ap hidden
 * @param bool   wifi ap hidden, default is false
 */
void AsyncWiFiManager::setWiFiAPHidden(bool hidden)
{
    _apHidden = hidden;
}

/**
 * toggle showing erase wifi config button on info page
 * @param boolean enabled
 */
void AsyncWiFiManager::setShowInfoErase(boolean enabled)
{
    _showInfoErase = enabled;
}

/**
 * toggle showing update upload web ota button on info page
 * @param boolean enabled
 */
void AsyncWiFiManager::setShowInfoUpdate(boolean enabled)
{
    _showInfoUpdate = enabled;
}

/**
 * check if the config portal is running
 * @return bool true if active
 */
bool AsyncWiFiManager::getConfigPortalActive()
{
    return configPortalActive;
}

/**
 * [getConfigPortalActive description]
 * @return bool true if active
 */
bool AsyncWiFiManager::getWebPortalActive()
{
    return webPortalActive;
}

String AsyncWiFiManager::getWiFiHostname()
{
#ifdef ESP32
    return (String)WiFi.getHostname();
#else
    return (String)WiFi.hostname();
#endif
}

/**
 * [setTitle description]
 * @param String title, set app title
 */
void AsyncWiFiManager::setTitle(String title)
{
    _title = title;
}

/**
 * set menu items and order
 * if param is present in menu , params will be removed from wifi page automatically
 * eg.
 *  const char * menu[] = {"wifi","setup","sep","info","exit"};
 *  AsyncWiFiManager.setMenu(menu);
 * @since $dev
 * @param uint8_t menu[] array of menu ids
 */
void AsyncWiFiManager::setMenu(const char *menu[], uint8_t size)
{
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(DEBUG_VERBOSE,"setmenu array");
#endif
    _menuIds.clear();
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < _nummenutokens; j++)
        {
            if (menu[i] == _menutokens[j])
            {
                if ((String)menu[i] == "param")
                    _paramsInWifi = false; // param auto flag
                _menuIds.push_back(j);
            }
        }
    }
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(getMenuOut());
#endif
}

/**
 * setMenu with vector
 * eg.
 * std::vector<const char *> menu = {"wifi","setup","sep","info","exit"};
 * AsyncWiFiManager.setMenu(menu);
 * tokens can be found in _menutokens array in strings_en.h
 * @shiftIncrement $dev
 * @param {[type]} std::vector<const char *>& menu [description]
 */
void AsyncWiFiManager::setMenu(std::vector<const char *> &menu)
{
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(DEBUG_VERBOSE,"setmenu vector");
#endif
    _menuIds.clear();
    for (auto menuitem : menu)
    {
        for (size_t j = 0; j < _nummenutokens; j++)
        {
            if (menuitem == _menutokens[j])
            {
                if ((String)menuitem == "param")
                    _paramsInWifi = false; // param auto flag
                _menuIds.push_back(j);
            }
        }
    }
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(getMenuOut());
#endif
}

/**
 * set params as sperate page not in wifi
 * NOT COMPATIBLE WITH setMenu! 
 * @todo scan menuids and insert param after wifi or something, same for ota
 * @param bool enable 
 * @since $dev
 */
void AsyncWiFiManager::setParamsPage(bool enable)
{
    _paramsInWifi = !enable;
    setMenu(enable ? _menuIdsParams : _menuIdsDefault);
}

// GETTERS

/**
 * get config portal AP SSID
 * @since 0.0.1
 * @access public
 * @return String the configportal ap name
 */
String AsyncWiFiManager::getConfigPortalSSID()
{
    return _apName;
}

/**
 * return the last known connection result
 * logged on autoconnect and wifisave, can be used to check why failed
 * get as readable string with getWLStatusString(getLastConxResult);
 * @since $dev
 * @access public
 * @return bool return wl_status codes
 */
uint8_t AsyncWiFiManager::getLastConxResult()
{
    return _lastconxresult;
}

/**
 * check if wifi has a saved ap or not
 * @since $dev
 * @access public
 * @return bool true if a saved ap config exists
 */
bool AsyncWiFiManager::getWiFiIsSaved()
{
    return WiFi_hasAutoConnect();
}

String AsyncWiFiManager::getDefaultAPName()
{
    String hostString = String(WIFI_getChipId(), HEX);
    hostString.toUpperCase();
    // char hostString[16] = {0};
    // sprintf(hostString, "%06X", ESP.getChipId());
    return _wifissidprefix + "_" + hostString;
}

/**
 * setCountry
 * @since $dev
 * @param String cc country code, must be defined in WiFiSetCountry, US, JP, CN
 */
void AsyncWiFiManager::setCountry(String cc)
{
    _wificountry = cc;
}

/**
 * setClass
 * @param String str body class string
 */
void AsyncWiFiManager::setClass(String str)
{
    _bodyClass = str;
}

/**
 * setDarkMode
 * @param bool enable, enable dark mode via invert class
 */
void AsyncWiFiManager::setDarkMode(bool enable)
{
    _bodyClass = enable ? "invert" : "";
}

/**
 * setHttpPort
 * @param uint16_t port webserver port number default 80
 */
void AsyncWiFiManager::setHttpPort(uint16_t port)
{
    _httpPort = port;
}

bool AsyncWiFiManager::preloadWiFi(String ssid, String pass)
{
    _defaultssid = ssid;
    _defaultpass = pass;
    return true;
}

// HELPERS

/**
 * getWiFiSSID
 * @since $dev
 * @param bool persistent
 * @return String
 */
String AsyncWiFiManager::getWiFiSSID(bool persistent)
{
    return WiFi_SSID(persistent);
}

/**
 * getWiFiPass
 * @since $dev
 * @param bool persistent
 * @return String
 */
String AsyncWiFiManager::getWiFiPass(bool persistent)
{
    return WiFi_psk(persistent);
}

// DEBUG
// @todo fix DEBUG_AWM(0,0);
template <typename Generic>
void AsyncWiFiManager::DEBUG_AWM(Generic text)
{
    DEBUG_AWM(DEBUG_NOTIFY, text, "");
}

template <typename Generic>
void AsyncWiFiManager::DEBUG_AWM(awm_debuglevel_t level, Generic text)
{
    if (_debugLevel >= level)
        DEBUG_AWM(level, text, "");
}

template <typename Generic, typename Genericb>
void AsyncWiFiManager::DEBUG_AWM(Generic text, Genericb textb)
{
    DEBUG_AWM(DEBUG_NOTIFY, text, textb);
}

template <typename Generic, typename Genericb>
void AsyncWiFiManager::DEBUG_AWM(awm_debuglevel_t level, Generic text, Genericb textb, bool cr)
{
    if (!_debug || _debugLevel < level)
        return;

    if (_debugLevel >= DEBUG_MAX)
    {
#ifdef ESP8266
// uint32_t free;
// uint16_t max;
// uint8_t frag;
// ESP.getHeapStats(&free, &max, &frag);// @todo Does not exist in 2.3.0
// _debugPort.printf("[MEM] free: %5d | max: %5d | frag: %3d%% \n", free, max, frag);
#elif defined ESP32
        // total_free_bytes;      ///<  Total free bytes in the heap. Equivalent to multi_free_heap_size().
        // total_allocated_bytes; ///<  Total bytes allocated to data in the heap.
        // largest_free_block;    ///<  Size of largest free block in the heap. This is the largest malloc-able size.
        // minimum_free_bytes;    ///<  Lifetime minimum free heap size. Equivalent to multi_minimum_free_heap_size().
        // allocated_blocks;      ///<  Number of (variable size) blocks allocated in the heap.
        // free_blocks;           ///<  Number of (variable size) free blocks in the heap.
        // total_blocks;          ///<  Total number of (variable size) blocks in the heap.
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
        uint32_t free = info.total_free_bytes;
        uint16_t max = info.largest_free_block;
        uint8_t frag = 100 - (max * 100) / free;
        _debugPort.printf("[MEM] free: %5d | max: %5d | frag: %3d%% %s", free, max, frag, cr ? "\n" : "");
#endif
    }
    _debugPort.print(_debugPrefix);
    if (_debugLevel >= debugLvlShow)
        _debugPort.print("[" + (String)level + "] ");
    _debugPort.print(text);
    if (textb)
    {
        _debugPort.print(" ");
        _debugPort.print(textb);
    }
    _debugPort.print(cr ? "\n" : ""); // Allow no \n after line
}

/**
 * [debugSoftAPConfig description]
 * @access public
 * @return {[type]} [description]
 */
void AsyncWiFiManager::debugSoftAPConfig()
{

#ifdef ESP8266
    softap_config config;
    wifi_softap_get_config(&config);
#if !defined(WM_NOCOUNTRY)
    wifi_country_t country;
    wifi_get_country(&country);
#endif
#elif defined(ESP32)
    wifi_country_t country;
    wifi_config_t conf_config;
    esp_wifi_get_config(WIFI_IF_AP, &conf_config); // == ESP_OK
    wifi_ap_config_t config = conf_config.ap;
    esp_wifi_get_country(&country);
#endif

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("SoftAP Configuration"));
    DEBUG_AWM(FPSTR(D_HR));
    DEBUG_AWM(F("ssid:            "), (char *)config.ssid);
    DEBUG_AWM(F("password:        "), (char *)config.password);
    DEBUG_AWM(F("ssid_len:        "), config.ssid_len);
    DEBUG_AWM(F("channel:         "), config.channel);
    DEBUG_AWM(F("authmode:        "), config.authmode);
    DEBUG_AWM(F("ssid_hidden:     "), config.ssid_hidden);
    DEBUG_AWM(F("max_connection:  "), config.max_connection);
#endif
#if !defined(WM_NOCOUNTRY)
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("country:         "), (String)country.cc);
#endif
    DEBUG_AWM(F("beacon_interval: "), (String)config.beacon_interval + "(ms)");
    DEBUG_AWM(FPSTR(D_HR));
#endif
}

/**
 * [debugPlatformInfo description]
 * @access public
 * @return {[type]} [description]
 */
void AsyncWiFiManager::debugPlatformInfo()
{
#ifdef ESP8266
    system_print_meminfo();
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("getCoreVersion():         "), ESP.getCoreVersion());
    DEBUG_AWM(F("system_get_sdk_version(): "), system_get_sdk_version());
    DEBUG_AWM(F("system_get_boot_version():"), system_get_boot_version());
    DEBUG_AWM(F("getFreeHeap():            "), (String)ESP.getFreeHeap());
#endif
#elif defined(ESP32)
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(F("Free heap:       "), ESP.getFreeHeap());
    DEBUG_AWM(F("ESP SDK version: "), ESP.getSdkVersion());
#endif
// esp_chip_info_t chipInfo;
// esp_chip_info(&chipInfo);
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM("Chip Info: Model: ",chipInfo.model);
// DEBUG_AWM("Chip Info: Cores: ",chipInfo.cores);
// DEBUG_AWM("Chip Info: Rev: ",chipInfo.revision);
// DEBUG_AWM(printf("Chip Info: Model: %d, cores: %d, revision: %d", chipInfo.model.c_str(), chipInfo.cores, chipInfo.revision));
// DEBUG_AWM("Chip Rev: ",(String)ESP.getChipRevision());
#endif
    // core version is not avail
#endif
}

int AsyncWiFiManager::getRSSIasQuality(int RSSI)
{
    int quality = 0;

    if (RSSI <= -100)
    {
        quality = 0;
    }
    else if (RSSI >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}

/** Is this an IP? */
boolean AsyncWiFiManager::isIp(String str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String AsyncWiFiManager::toStringIp(IPAddress ip)
{
    String res = "";
    for (int i = 0; i < 3; i++)
    {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

boolean AsyncWiFiManager::validApPassword()
{
    // check that ap password is valid, return false
    if (_apPassword == NULL)
        _apPassword = "";
    if (_apPassword != "")
    {
        if (_apPassword.length() < 8 || _apPassword.length() > 63)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(F("AccessPoint set password is INVALID or <8 chars"));
#endif
            _apPassword = "";
            return false; // @todo FATAL or fallback to empty ?
        }
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("AccessPoint set password is VALID"));
        DEBUG_AWM(DEBUG_DEV, "ap pass", _apPassword);
#endif
    }
    return true;
}

/**
 * encode htmlentities
 * @since $dev
 * @param  string str  string to replace entities
 * @return string      encoded string
 */
String AsyncWiFiManager::htmlEntities(String str)
{
    str.replace("&", "&amp;");
    str.replace("<", "&lt;");
    str.replace(">", "&gt;");
    // str.replace("'","&#39;");
    // str.replace("\"","&quot;");
    // str.replace("/": "&#x2F;");
    // str.replace("`": "&#x60;");
    // str.replace("=": "&#x3D;");
    return str;
}

/**
 * [getWLStatusString description]
 * @access public
 * @param  {[type]} uint8_t status        [description]
 * @return {[type]}         [description]
 */
String AsyncWiFiManager::getWLStatusString(uint8_t status)
{
    if (status <= 7)
        return WIFI_STA_STATUS[status];
    return FPSTR(S_NA);
}

String AsyncWiFiManager::encryptionTypeStr(uint8_t authmode)
{
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM("enc_tye: ",authmode);
#endif
    return AUTH_MODE_NAMES[authmode];
}

String AsyncWiFiManager::getModeString(uint8_t mode)
{
    if (mode <= 3)
        return WIFI_MODES[mode];
    return FPSTR(S_NA);
}

bool AsyncWiFiManager::WiFiSetCountry()
{
    if (_wificountry == "")
        return false; // skip not set

#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("WiFiSetCountry to"), _wificountry);
#endif

    /*
  * @return
  *    - ESP_OK: succeed
  *    - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by eps_wifi_init
  *    - ESP_ERR_WIFI_IF: invalid interface
  *    - ESP_ERR_WIFI_ARG: invalid argument
  *    - others: refer to error codes in esp_err.h
  */

    // @todo move these definitions, and out of cpp `esp_wifi_set_country(&WM_COUNTRY_US)`
    bool ret = true;
// ret = esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20); // WIFI_BW_HT40
#ifdef ESP32
    esp_err_t err = ESP_OK;
    // @todo check if wifi is init, no idea how, doesnt seem to be exposed atm ( might be now! )
    if (WiFi.getMode() == WIFI_MODE_NULL)
    {
        DEBUG_AWM(DEBUG_ERROR, "[ERROR] cannot set country, wifi not init");
    } // exception if wifi not init!
    else if (_wificountry == "US")
        err = esp_wifi_set_country(&WM_COUNTRY_US);
    else if (_wificountry == "JP")
        err = esp_wifi_set_country(&WM_COUNTRY_JP);
    else if (_wificountry == "CN")
        err = esp_wifi_set_country(&WM_COUNTRY_CN);
#ifdef AWM_DEBUG_LEVEL
    else
    {
        DEBUG_AWM(DEBUG_ERROR, "[ERROR] country code not found");
    }
    if (err)
    {
        if (err == ESP_ERR_WIFI_NOT_INIT)
            DEBUG_AWM(DEBUG_ERROR, "[ERROR] ESP_ERR_WIFI_NOT_INIT");
        else if (err == ESP_ERR_INVALID_ARG)
            DEBUG_AWM(DEBUG_ERROR, "[ERROR] ESP_ERR_WIFI_ARG");
        else if (err != ESP_OK)
            DEBUG_AWM(DEBUG_ERROR, "[ERROR] unknown error", (String)err);
    }
#endif
    ret = err == ESP_OK;

#elif defined(ESP8266) && !defined(WM_NOCOUNTRY)
    // if(WiFi.getMode() == WIFI_OFF); // exception if wifi not init!
    if (_wificountry == "US")
        ret = wifi_set_country((wifi_country_t *)&WM_COUNTRY_US);
    else if (_wificountry == "JP")
        ret = wifi_set_country((wifi_country_t *)&WM_COUNTRY_JP);
    else if (_wificountry == "CN")
        ret = wifi_set_country((wifi_country_t *)&WM_COUNTRY_CN);
#ifdef AWM_DEBUG_LEVEL
    else
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] country code not found"));
#endif
#endif

#ifdef AWM_DEBUG_LEVEL
    if (ret)
        DEBUG_AWM(DEBUG_VERBOSE, F("[OK] esp_wifi_set_country: "), _wificountry);
    else
        DEBUG_AWM(DEBUG_ERROR, F("[ERROR] esp_wifi_set_country failed"));
#endif
    return ret;
}

// set mode ignores WiFi.persistent
bool AsyncWiFiManager::WiFi_Mode(WiFiMode_t m, bool persistent)
{
    bool ret;
#ifdef ESP8266
    if ((wifi_get_opmode() == (uint8)m) && !persistent)
    {
        return true;
    }
    ETS_UART_INTR_DISABLE();
    if (persistent)
        ret = wifi_set_opmode(m);
    else
        ret = wifi_set_opmode_current(m);
    ETS_UART_INTR_ENABLE();
    return ret;
#elif defined(ESP32)
    if (persistent && esp32persistent)
        WiFi.persistent(true);
    ret = WiFi.mode(m); // @todo persistent check persistant mode , NI
    if (persistent && esp32persistent)
        WiFi.persistent(false);
    return ret;
#endif
}
bool AsyncWiFiManager::WiFi_Mode(WiFiMode_t m)
{
    return WiFi_Mode(m, false);
}

// sta disconnect without persistent
bool AsyncWiFiManager::WiFi_Disconnect()
{
#ifdef ESP8266
    if ((WiFi.getMode() & WIFI_STA) != 0)
    {
        bool ret;
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_DEV, F("WiFi station disconnect"));
#endif
        ETS_UART_INTR_DISABLE(); // @todo probably not needed
        ret = wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
        return ret;
    }
#elif defined(ESP32)
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("WiFi station disconnect"));
#endif
    return WiFi.disconnect(); // not persistent atm
#endif
    return false;
}

// toggle STA without persistent
bool AsyncWiFiManager::WiFi_enableSTA(bool enable, bool persistent)
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_DEV, F("WiFi_enableSTA"), (String)enable ? "enable" : "disable");
#endif
#ifdef ESP8266
    WiFiMode_t newMode;
    WiFiMode_t currentMode = WiFi.getMode();
    bool isEnabled = (currentMode & WIFI_STA) != 0;
    if (enable)
        newMode = (WiFiMode_t)(currentMode | WIFI_STA);
    else
        newMode = (WiFiMode_t)(currentMode & (~WIFI_STA));

    if ((isEnabled != enable) || persistent)
    {
        if (enable)
        {
#ifdef AWM_DEBUG_LEVEL
            if (persistent)
                DEBUG_AWM(DEBUG_DEV, F("enableSTA PERSISTENT ON"));
#endif
            return WiFi_Mode(newMode, persistent);
        }
        else
        {
            return WiFi_Mode(newMode, persistent);
        }
    }
    else
    {
        return true;
    }
#elif defined(ESP32)
    bool ret;
    if (persistent && esp32persistent)
        WiFi.persistent(true);
    ret = WiFi.enableSTA(enable); // @todo handle persistent when it is implemented in platform
    if (persistent && esp32persistent)
        WiFi.persistent(false);
    return ret;
#endif
}
bool AsyncWiFiManager::WiFi_enableSTA(bool enable)
{
    return WiFi_enableSTA(enable, false);
}

bool AsyncWiFiManager::WiFi_eraseConfig()
{
#ifdef ESP8266
#ifndef WM_FIXERASECONFIG
    return ESP.eraseConfig();
#else
    // erase config BUG replacement
    // https://github.com/esp8266/Arduino/pull/3635
    const size_t cfgSize = 0x4000;
    size_t cfgAddr = ESP.getFlashChipSize() - cfgSize;

    for (size_t offset = 0; offset < cfgSize; offset += SPI_FLASH_SEC_SIZE)
    {
        if (!ESP.flashEraseSector((cfgAddr + offset) / SPI_FLASH_SEC_SIZE))
        {
            return false;
        }
    }
    return true;
#endif
#elif defined(ESP32)
    bool ret;
    WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
    WiFi.persistent(true);
    ret = WiFi.disconnect(true, true);
    WiFi.persistent(false);
    return ret;
#endif
}

uint8_t AsyncWiFiManager::WiFi_softap_num_stations()
{
#ifdef ESP8266
    return wifi_softap_get_station_num();
#elif defined(ESP32)
    return WiFi.softAPgetStationNum();
#endif
}

bool AsyncWiFiManager::WiFi_hasAutoConnect()
{
    return WiFi_SSID(true) != "";
}

String AsyncWiFiManager::WiFi_SSID(bool persistent) const
{

#ifdef ESP8266
    struct station_config conf;
    if (persistent)
        wifi_station_get_config_default(&conf);
    else
        wifi_station_get_config(&conf);

    char tmp[33]; //ssid can be up to 32chars, => plus null term
    memcpy(tmp, conf.ssid, sizeof(conf.ssid));
    tmp[32] = 0; //nullterm in case of 32 char ssid
    return String(reinterpret_cast<char *>(tmp));

#elif defined(ESP32)
    if (persistent)
    {
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        return String(reinterpret_cast<const char *>(conf.sta.ssid));
    }
    else
    {
        if (WiFiGenericClass::getMode() == WIFI_MODE_NULL)
        {
            return String();
        }
        wifi_ap_record_t info;
        if (!esp_wifi_sta_get_ap_info(&info))
        {
            return String(reinterpret_cast<char *>(info.ssid));
        }
        return String();
    }
#endif
}

String AsyncWiFiManager::WiFi_psk(bool persistent) const
{
#ifdef ESP8266
    struct station_config conf;

    if (persistent)
        wifi_station_get_config_default(&conf);
    else
        wifi_station_get_config(&conf);

    char tmp[65]; //psk is 64 bytes hex => plus null term
    memcpy(tmp, conf.password, sizeof(conf.password));
    tmp[64] = 0; //null term in case of 64 byte psk
    return String(reinterpret_cast<char *>(tmp));

#elif defined(ESP32)
    // only if wifi is init
    if (WiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return String();
    }
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return String(reinterpret_cast<char *>(conf.sta.password));
#endif
}

#ifdef ESP32
void AsyncWiFiManager::WiFiEvent(WiFiEvent_t event, system_event_info_t info)
{
    if (!_hasBegun)
    {
#ifdef AWM_DEBUG_LEVEL
        // DEBUG_AWM(DEBUG_VERBOSE,"[ERROR] WiFiEvent, not ready");
#endif
        // Serial.println(F("\n[EVENT] WiFiEvent logging (wm debug not available)"));
        // Serial.print(F("[EVENT] ID: "));
        // Serial.println(event);
        return;
    }
#ifdef AWM_DEBUG_LEVEL
// DEBUG_AWM(DEBUG_VERBOSE,"[EVENT]",event);
#endif
    if (event == SYSTEM_EVENT_STA_DISCONNECTED)
    {
#ifdef AWM_DEBUG_LEVEL
        Serial.println();
        DEBUG_AWM(DEBUG_VERBOSE, F("[EVENT] WIFI_REASON: "), info.disconnected.reason);
#endif
        if (info.disconnected.reason == WIFI_REASON_AUTH_EXPIRE || info.disconnected.reason == WIFI_REASON_AUTH_FAIL)
        {
            _lastconxresulttmp = 7; // hack in wrong password internally, sdk emit WIFI_REASON_AUTH_EXPIRE on some routers on auth_fail
        }
        else if (info.disconnected.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)
        {
            // Hack to reset due to ESP not connecting after flashing
            DEBUG_WM(DEBUG_VERBOSE, F("[EVENT] WIFI_REASON: WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT detected, resetting."));
            ESP.restart();
        }
        else
            _lastconxresulttmp = WiFi.status();
#ifdef AWM_DEBUG_LEVEL
        if (info.disconnected.reason == WIFI_REASON_NO_AP_FOUND)
            DEBUG_AWM(DEBUG_VERBOSE, F("[EVENT] WIFI_REASON: NO_AP_FOUND"));
#endif
#ifdef esp32autoreconnect
#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, F("[Event] SYSTEM_EVENT_STA_DISCONNECTED, reconnecting"));
#endif
        WiFi.reconnect();
#endif
    }
    else if (event == SYSTEM_EVENT_SCAN_DONE)
    {
        uint16_t scans = WiFi.scanComplete();
        WiFi_scanComplete(scans);
    }
}
#endif

void AsyncWiFiManager::WiFi_autoReconnect()
{
#ifdef ESP8266
    WiFi.setAutoReconnect(_wifiAutoReconnect);
#elif defined(ESP32)
    // if(_wifiAutoReconnect){
    // @todo move to seperate method, used for event listener now
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("ESP32 event handler enabled"));
#endif
    using namespace std::placeholders;
    wm_event_id = WiFi.onEvent(std::bind(&AsyncWiFiManager::WiFiEvent, this, _1, _2));
    // }
#endif
}

// Called when /update is requested
void AsyncWiFiManager::handleUpdate()
{
#ifdef AWM_DEBUG_LEVEL
    DEBUG_AWM(DEBUG_VERBOSE, F("<- Handle update"));
#endif
    if (captivePortal())
        return;                        // If captive portal redirect instead of displaying the page
    String page = getHTTPHead(_title); // @token options
    String str = FPSTR(HTTP_ROOT_MAIN);
    str.replace(FPSTR(T_t), _title);
    str.replace(FPSTR(T_v), configPortalActive ? _apName : (getWiFiHostname() + " - " + WiFi.localIP().toString())); // use ip if ap is not active for heading
    page += str;

    page += FPSTR(HTTP_UPDATE);
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);
}

// upload via /u POST
void AsyncWiFiManager::handleUpdating()
{
    // @todo
    // cannot upload files in captive portal, file select is not allowed, show message with link or hide
    // cannot upload if softreset after upload, maybe check for hard reset at least for dev, ERROR[11]: Invalid bootstrapping state, reset ESP8266 before updating
    // add upload status to webpage somehow
    // abort upload if error detected ?
    // [x] supress cp timeout on upload, so it doesnt keep uploading?
    // add progress handler for debugging
    // combine route handlers into one callback and use argument or post checking instead of mutiple functions maybe, if POST process else server upload page?
    // [x] add upload checking, do we need too check file?
    // convert output to debugger if not moving to example
    if (captivePortal())
        return; // If captive portal redirect instead of displaying the page
    bool error = false;
    unsigned long _configPortalTimeoutSAV = _configPortalTimeout; // store cp timeout
    _configPortalTimeout = 0;                                     // disable timeout

    // handler for the file upload, get's the sketch bytes, and writes
    // them through the Update object
    HTTPUpload &upload = server->upload();

    // UPLOAD START
    if (upload.status == UPLOAD_FILE_START)
    {
        if (_debug)
            Serial.setDebugOutput(true);
        uint32_t maxSketchSpace;

#ifdef ESP8266
        WiFiUDP::stopAll();
        maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#elif defined(ESP32)
        // Think we do not need to stop WiFIUDP because we haven't started a listener
        // maxSketchSpace = (ESP.getFlashChipSize() - 0x1000) & 0xFFFFF000;
        // #define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF // include update.h
        maxSketchSpace = UPDATE_SIZE_UNKNOWN;
#endif

#ifdef AWM_DEBUG_LEVEL
        DEBUG_AWM(DEBUG_VERBOSE, "Update file: ", upload.filename.c_str());
#endif

        if (!Update.begin(maxSketchSpace))
        { // start with max available size
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] OTA Update ERROR"), Update.getError());
#endif
            error = true;
            Update.end(); // Not sure the best way to abort, I think client will keep sending..
        }
    }
    // UPLOAD WRITE
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        Serial.print(".");
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_ERROR, F("[ERROR] OTA Update WRITE ERROR"), Update.getError());
//Update.printError(Serial); // write failure
#endif
            error = true;
        }
    }
    // UPLOAD FILE END
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        { // true to set the size to the current progress
#ifdef AWM_DEBUG_LEVEL
            DEBUG_AWM(DEBUG_VERBOSE, F("\n\n[OTA] OTA FILE END bytes: "), upload.totalSize);
// Serial.printf("Updated: %u bytes\r\nRebooting...\r\n", upload.totalSize);
#endif
        }
        else
        {
            Update.printError(Serial);
            error = true;
        }
    }
    // UPLOAD ABORT
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        Update.end();
        DEBUG_AWM(F("[OTA] Update was aborted"));
        error = true;
    }
    if (error)
        _configPortalTimeout = _configPortalTimeoutSAV;
    delay(0);
}

// upload and ota done, show status
void AsyncWiFiManager::handleUpdateDone()
{
    DEBUG_AWM(DEBUG_VERBOSE, F("<- Handle update done"));
    if (captivePortal())
        return; // If captive portal redirect instead of displaying the page

    String page = getHTTPHead(FPSTR(S_options)); // @token options
    String str = FPSTR(HTTP_ROOT_MAIN);
    str.replace(FPSTR(T_t), _title);
    str.replace(FPSTR(T_v), configPortalActive ? _apName : WiFi.localIP().toString()); // use ip if ap is not active for heading
    page += str;

    if (Update.hasError())
    {
        page += FPSTR(HTTP_UPDATE_FAIL);
#ifdef ESP32
        page += "OTA Error: " + (String)Update.errorString();
#else
        page += "OTA Error: " + (String)Update.getError();
#endif
        DEBUG_AWM(F("[OTA] update failed"));
    }
    else
    {
        page += FPSTR(HTTP_UPDATE_SUCCESS);
        DEBUG_AWM(F("[OTA] update ok"));
    }
    page += FPSTR(HTTP_END);

    // server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
    server->send(200, FPSTR(HTTP_HEAD_CT), page);

    delay(1000); // send page
    if (!Update.hasError())
    {
        ESP.restart();
    }
}

#endif