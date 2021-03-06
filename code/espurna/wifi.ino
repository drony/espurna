/*

WIFI MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "JustWifi.h"

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------

void _wifiWebSocketOnSend(JsonObject& root) {
    root["maxNetworks"] = WIFI_MAX_NETWORKS;
    JsonArray& wifi = root.createNestedArray("wifi");
    for (byte i=0; i<WIFI_MAX_NETWORKS; i++) {
        if (!hasSetting("ssid", i)) break;
        JsonObject& network = wifi.createNestedObject();
        network["ssid"] = getSetting("ssid", i, "");
        network["pass"] = getSetting("pass", i, "");
        network["ip"] = getSetting("ip", i, "");
        network["gw"] = getSetting("gw", i, "");
        network["mask"] = getSetting("mask", i, "");
        network["dns"] = getSetting("dns", i, "");
    }
}

String getIP() {
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String getNetwork() {
    if (WiFi.getMode() == WIFI_AP) {
        return jw.getAPSSID();
    }
    return WiFi.SSID();
}

void wifiDisconnect() {
    jw.disconnect();
}

void resetConnectionTimeout() {
    jw.resetReconnectTimeout();
}

bool wifiConnected() {
    return jw.connected();
}

bool createAP() {
    jw.disconnect();
    jw.resetReconnectTimeout();
    return jw.createAP();
}

void wifiReconnectCheck() {
    bool connected = false;
    #if WEB_SUPPORT
        if (wsConnected()) connected = true;
    #endif
    #if TELNET_SUPPORT
        if (telnetConnected()) connected = true;
    #endif
    jw.setReconnectTimeout(connected ? 0 : WIFI_RECONNECT_INTERVAL);
}

void wifiConfigure() {

    jw.setHostname(getSetting("hostname").c_str());
    jw.setSoftAP(getSetting("hostname").c_str(), getSetting("adminPass", ADMIN_PASS).c_str());
    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.setAPMode(WIFI_AP_MODE);
    jw.cleanNetworks();

    // If system is flagged unstable we do not init wifi networks
    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    int i;
    for (i = 0; i< WIFI_MAX_NETWORKS; i++) {
        if (getSetting("ssid" + String(i)).length() == 0) break;
        if (getSetting("ip" + String(i)).length() == 0) {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str()
            );
        } else {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str(),
                getSetting("ip" + String(i)).c_str(),
                getSetting("gw" + String(i)).c_str(),
                getSetting("mask" + String(i)).c_str(),
                getSetting("dns" + String(i)).c_str()
            );
        }
    }

    // Scan for best network only if we have more than 1 defined
    jw.scanNetworks(i>1);

}

void wifiStatus() {

    if (WiFi.getMode() == WIFI_AP_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP + STA --------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_AP) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP --------------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE STA -------------------------------------\n"));
    } else {
        DEBUG_MSG_P(PSTR("[WIFI] MODE OFF -------------------------------------\n"));
        DEBUG_MSG_P(PSTR("[WIFI] No connection\n"));
    }

    if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
        DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), jw.getAPSSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] PASS %s\n"), getSetting("adminPass", ADMIN_PASS).c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.softAPmacAddress().c_str());
    }

    if ((WiFi.getMode() & WIFI_STA) == WIFI_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW   %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS  %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST %s\n"), WiFi.hostname().c_str());
    }

    DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));

}

bool wifiClean(unsigned char num) {

    bool changed = false;
    int i = 0;

    // Clean defined settings
    while (i < num) {

        // Skip on first non-defined setting
        if (!hasSetting("ssid", i)) {
            delSetting("ssid", i);
            break;
        }

        // Delete empty values
        if (!hasSetting("pass", i)) delSetting("pass", i);
        if (!hasSetting("ip", i)) delSetting("ip", i);
        if (!hasSetting("gw", i)) delSetting("gw", i);
        if (!hasSetting("mask", i)) delSetting("mask", i);
        if (!hasSetting("dns", i)) delSetting("dns", i);

        ++i;

    }

    // Delete all other settings
    while (i < WIFI_MAX_NETWORKS) {
        changed = hasSetting("ssid", i);
        delSetting("ssid", i);
        delSetting("pass", i);
        delSetting("ip", i);
        delSetting("gw", i);
        delSetting("mask", i);
        delSetting("dns", i);
        ++i;
    }

    return changed;

}

// Inject hardcoded networks
void wifiInject() {

    #ifdef WIFI1_SSID
        if (getSetting("ssid", 0, "").length() == 0) setSetting("ssid", 0, WIFI1_SSID);
    #endif
    #ifdef WIFI1_PASS
        if (getSetting("pass", 0, "").length() == 0) setSetting("pass", 0, WIFI1_PASS);
    #endif
    #ifdef WIFI1_IP
        if (getSetting("ip", 0, "").length() == 0) setSetting("ip", 0, WIFI1_IP);
    #endif
    #ifdef WIFI1_GW
        if (getSetting("gw", 0, "").length() == 0) setSetting("gw", 0, WIFI1_GW);
    #endif
    #ifdef WIFI1_MASK
        if (getSetting("mask", 0, "").length() == 0) setSetting("mask", 0, WIFI1_MASK);
    #endif
    #ifdef WIFI1_DNS
        if (getSetting("dns", 0, "").length() == 0) setSetting("dns", 0, WIFI1_DNS);
    #endif

    #ifdef WIFI2_SSID
        if (getSetting("ssid", 1, "").length() == 0) setSetting("ssid", 1, WIFI2_SSID);
    #endif
    #ifdef WIFI2_PASS
        if (getSetting("pass", 1, "").length() == 0) setSetting("pass", 1, WIFI2_PASS);
    #endif
    #ifdef WIFI2_IP
        if (getSetting("ip", 1, "").length() == 0) setSetting("ip", 1, WIFI2_IP);
    #endif
    #ifdef WIFI2_GW
        if (getSetting("gw", 1, "").length() == 0) setSetting("gw", 1, WIFI2_GW);
    #endif
    #ifdef WIFI2_MASK
        if (getSetting("mask", 1, "").length() == 0) setSetting("mask", 1, WIFI2_MASK);
    #endif
    #ifdef WIFI2_DNS
        if (getSetting("dns", 1, "").length() == 0) setSetting("dns", 1, WIFI2_DNS);
    #endif

}

void wifiSetup() {

    #if WIFI_SLEEP_ENABLED
        wifi_set_sleep_type(LIGHT_SLEEP_T);
    #endif

    wifiInject();
    wifiConfigure();

    // Message callbacks
    jw.onMessage([](justwifi_messages_t code, char * parameter) {

		#if DEBUG_SUPPORT

		    if (code == MESSAGE_SCANNING) {
		        DEBUG_MSG_P(PSTR("[WIFI] Scanning\n"));
		    }

		    if (code == MESSAGE_SCAN_FAILED) {
		        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
		    }

		    if (code == MESSAGE_NO_NETWORKS) {
		        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
		    }

		    if (code == MESSAGE_NO_KNOWN_NETWORKS) {
		        DEBUG_MSG_P(PSTR("[WIFI] No known networks found\n"));
		    }

		    if (code == MESSAGE_FOUND_NETWORK) {
		        DEBUG_MSG_P(PSTR("[WIFI] %s\n"), parameter);
		    }

		    if (code == MESSAGE_CONNECTING) {
		        DEBUG_MSG_P(PSTR("[WIFI] Connecting to %s\n"), parameter);
		    }

		    if (code == MESSAGE_CONNECT_WAITING) {
		        // too much noise
		    }

		    if (code == MESSAGE_CONNECT_FAILED) {
		        DEBUG_MSG_P(PSTR("[WIFI] Could not connect to %s\n"), parameter);
		    }

		    if (code == MESSAGE_CONNECTED) {
                wifiStatus();
		    }

		    if (code == MESSAGE_ACCESSPOINT_CREATED) {
                wifiStatus();
		    }

		    if (code == MESSAGE_DISCONNECTED) {
		        DEBUG_MSG_P(PSTR("[WIFI] Disconnected\n"));
		    }

		    if (code == MESSAGE_ACCESSPOINT_CREATING) {
		        DEBUG_MSG_P(PSTR("[WIFI] Creating access point\n"));
		    }

		    if (code == MESSAGE_ACCESSPOINT_FAILED) {
		        DEBUG_MSG_P(PSTR("[WIFI] Could not create access point\n"));
		    }

		#endif // DEBUG_SUPPORT

        #if MQTT_SUPPORT
        #if MDNS_SUPPORT
            if (code == MESSAGE_CONNECTED) mdnsFindMQTT();
        #endif // MDNS_SUPPORT
        #endif // MQTT_SUPPORT

    });

    #if WEB_SUPPORT
        wsOnSendRegister(_wifiWebSocketOnSend);
        wsOnAfterParseRegister(wifiConfigure);
    #endif

}

void wifiLoop() {
    jw.loop();
}
