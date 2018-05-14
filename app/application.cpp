#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "Nexus 5" // Put you SSID and Password here
	#define WIFI_PWD "fadli1987"
#endif

#define LED_PIN 2 // GPIO2

Timer procTimer;
bool state = true;

// Blink the LED while not connected
void blink()
{
	digitalWrite(LED_PIN,state);
	state = !state;
}

// Will be called when WiFi station network scan was completed
void listNetworks(bool succeeded, BssList list)
{
	if (!succeeded)
	{
		Serial.println("Failed to scan networks");
		procTimer.initializeMs(1000, blink).start();
		return;
	}

	for (int i = 0; i < list.count(); i++)
	{
		Serial.print("\tWiFi: ");
		Serial.print(list[i].ssid);
		Serial.print(", ");
		Serial.print(list[i].getAuthorizationMethodName());
		if (list[i].hidden) Serial.print(" (hidden)");
		Serial.println();
	}
}


// Will be called when WiFi station was connected to AP
void connectOk(IPAddress ip, IPAddress mask, IPAddress gateway)
{
	debugf("I'm CONNECTED");
	procTimer.stop();
	digitalWrite(LED_PIN,HIGH);
	Serial.println(ip.toString());
}

// Will be called when WiFi station was disconnected
void connectFail(String ssid, uint8_t ssidLength, uint8_t *bssid, uint8_t reason)
{
	// The different reason codes can be found in user_interface.h. in your SDK.
	debugf("Disconnected from %s. Reason: %d", ssid.c_str(), reason);
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Sming. Let's do smart things!");

	// Set system ready callback method
	System.onReady(ready);

	// Soft access point
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config("Sming InternetOfThings", "", AUTH_OPEN);

	// Station - WiFi client
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

	// Optional: Change IP addresses (and disable DHCP)
	WifiAccessPoint.setIP(IPAddress(192, 168, 2, 1));
	WifiStation.setIP(IPAddress(192, 168, 1, 171));


	// Print available access points
	WifiStation.startScan(listNetworks); // In Sming we can start network scan from init method without additional code

	// Set callback that should be triggered when we have assigned IP
	WifiEvents.onStationGotIP(connectOk);

	// Set callback that should be triggered if we are disconnected or connection attempt failed
	WifiEvents.onStationDisconnect(connectFail);
}
