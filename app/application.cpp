#include <user_config.h>
#include <data_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/Http/Websocket/WebsocketResource.h>

#define LED_PIN 2 // GPIO2

Timer sendWS;

HttpServer server;

HttpServerSettings hss;

bool state = true;
bool serverStarted = false;
String responseJson = "";

StaticJsonBuffer<ConfigJsonBufferSize> jsonBuffer;

// Will be called when WiFi station network scan was completed
void listNetworks(bool succeeded, BssList list) {
	if (!succeeded)	{
		Serial.println("Failed to scan networks");
		return;
	}
	JsonArray& array = jsonBuffer.createArray();
	for (int i = 0; i < list.count(); i++) {
		// Serial.print("\tWiFi: ");
		// Serial.print(list[i].ssid);
		// Serial.print(", ");
		// Serial.print(list[i].getAuthorizationMethodName());
		// if (list[i].hidden) Serial.print(" (hidden)");
		// Serial.println();
		JsonObject& nestedSSID = array.createNestedObject();
		// JsonObject& nestedRSSI = array.createNestedObject();
		nestedSSID["SSID"] = (list[i].ssid);
		nestedSSID["RSSI"] = (list[i].rssi);
	}
		// array.printTo(responseJson);
	array.prettyPrintTo(Serial);
	// jsonBuffer = StaticJsonBuffer<ConfigJsonBufferSize>(); //clear memory so JSON won't overgrow memory

}


// Will be called when WiFi station was connected to AP
void connectOk(IPAddress ip, IPAddress mask, IPAddress gateway) {
	debugf("I'm CONNECTED");
	digitalWrite(LED_PIN,HIGH);
	Serial.println(ip.toString());
}

// Will be called when WiFi station was disconnected
void connectFail(String ssid, uint8_t ssidLength, uint8_t *bssid, uint8_t reason) {
	// The different reason codes can be found in user_interface.h. in your SDK.
	debugf("Disconnected from %s. Reason: %d", ssid.c_str(), reason);
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready() {
	debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
}

// Show info ESP8266 hardware info
void ShowInfo() {
    Serial.printf("\r\nSDK: v%s\r\n", system_get_sdk_version());
    Serial.printf("Free Heap: %d\r\n", system_get_free_heap_size());
    Serial.printf("CPU Frequency: %d MHz\r\n", system_get_cpu_freq());
    Serial.printf("System Chip ID: 0x%x\r\n", system_get_chip_id());
    Serial.printf("SPI Flash ID: 0x%x\r\n", spi_flash_get_id());
    // Serial.printf("SPI Flash Size: %d\r\n", (1 <> 16) & 0xff);
}

void onIndex(HttpRequest &request, HttpResponse &response) {
	response.setCache(86400, true); // It's important to use cache for better performance.
	response.sendFile("index.html");
}

void onFile(HttpRequest &request, HttpResponse &response) {
	String file = request.uri.Path;
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.code = HTTP_STATUS_FORBIDDEN;
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onWsConnect(WebSocketConnection& conn) {
	Serial.println("client connected");
	// String msg = "connected";
	// conn.broadcast(msg.c_str(), msg.length());		
}

void onWsDisconnect(WebSocketConnection& conn) {
	Serial.println("client disconnected");
	// String msg = "disconnected";
	// conn.broadcast(msg.c_str(), msg.length());	
}

void onWsMessage(WebSocketConnection& conn, const String& message) {
	Serial.print("got message: ");
	Serial.print(message);
	Serial.print(", free heap: ");
	Serial.println(system_get_free_heap_size());
	// String msg = "echo: " + message;
	// conn.send(msg.c_str(), msg.length());
	/*
	JsonObject& jsonParse = jsonRXBuffer.parseObject(message);
	if(jsonParse["get_aplist"]){
		Serial.println("Aplist true");
		WifiStation.startScan(listNetworks); 
		conn.send(responseJson.c_str(), responseJson.length());
	} */

	WifiStation.startScan(listNetworks);
	// Serial.print(responseJson);
	if (responseJson) conn.send(responseJson.c_str(), responseJson.length());
}

void configure_http() {
	hss.maxActiveConnections = 3;
	hss.minHeapSize = 0;
	hss.keepAliveSeconds = 0;
	hss.useDefaultBodyParsers = -1;
}

void startWebServer() {
	if (serverStarted) return;
	configure_http();
	server.configure(hss);
	server.listen(80);
	server.addPath("/", onIndex);
	server.setDefaultHandler(onFile);
	// server.addPath("/config", onConfiguration);
	// server.addPath("/config.json", onConfiguration_json);
	// server.addPath("/state", onAJAXGetState);

	WebsocketResource * res = new WebsocketResource();
	res->setConnectionHandler(onWsConnect);
	res->setMessageHandler(onWsMessage);
	res->setDisconnectionHandler(onWsDisconnect);
	server.addPath("/ws", res);

	serverStarted = true;

	if (WifiStation.isEnabled())
		debugf("STA: %s", WifiStation.getIP().toString().c_str());
	if (WifiAccessPoint.isEnabled())
		debugf("AP: %s", WifiAccessPoint.getIP().toString().c_str());
}

void init() {
	Serial.begin(SERIAL_BAUD_RATE);
	// Allow debug print to serial
	Serial.systemDebugOutput(true); 
	spiffs_mount();
	
	//SET higher CPU freq & disable wifi sleep
	system_update_cpu_freq(SYS_CPU_160MHZ);

	// Set system ready callback method
	System.onReady(ready);
	ShowInfo();

	// Soft access point
	WifiAccessPoint.enable(true);
	WifiAccessPoint.setIP(IPAddress(192, 168, 1, 1));
	WifiAccessPoint.config("ThingsX", "", AUTH_OPEN);

	// Station - WiFi client
	WifiStation.enable(true);	// for this moment ST mode will true
	// WifiStation.config(WIFI_SSID, WIFI_PWD); 

	// Set callback that should be triggered when we have assigned IP
	// WifiEvents.onStationGotIP(connectOk);

	// Set callback that should be triggered if we are disconnected or connection attempt failed
	// WifiEvents.onStationDisconnect(connectFail);

	startWebServer();
}
