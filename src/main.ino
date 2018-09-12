#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

/* for OLED display */
#include <OLEDDisplayUi.h>
#include <SSD1306Brzo.h>

/* for custom font, Roboto_Mono_6 */
#include "font.h"

/* bme280 device driver */
#include <TRB_BME280.h>
#include <brzo_i2c.h>

/* GPIO pins for I2C */
#if !defined(SCL)
#define SCL D3
#endif
#if !defined(SDA)
#define SDA D4
#endif

/* I2C addresses */
#if !defined(I2C_ADDRESS_BME280)
#define I2C_ADDRESS_BME280  0x76
#endif
#if !defined(I2C_ADDRESS_SSD1306)
#define I2C_ADDRESS_SSD1306  0x3C
#endif

/* Maximum line length of log buffer in character */
#define MAX_LINE_LENGTH (21)
/* Maximum number of lines of log buffer */
#define MAX_N_LINE  (5)

/* WiFi Connection manager, runs web configuration portal */
WiFiManager wifiManager;

/* bme280 device instance */
static struct bme280_dev dev;
/* data structure returned by the bme280 driver */
static struct bme280_data data;

/* OLEDDisplay instance */
SSD1306Brzo display(I2C_ADDRESS_SSD1306, SDA, SCL);
/* UI instance */
OLEDDisplayUi ui(&display);

void
halt()
{
	WiFi.disconnect(true); // remove saved connection
	Serial.println(F("Halting. Please reset"));
	while (1) {
		yield();
	}
}

int
i2c_init()
{
	Serial.println(F("Initializing I2C (brzo)"));
	brzo_i2c_setup(SDA, SCL, 200);
	bme280_brzo_set_scl_freq(400); // Khz
	return 0;
}

int
bme280_init()
{
	int err;
	struct bme280_settings settings;
	uint8_t settings_sel;

	/*
	 * BME280 settings. The values were chosen by balance between power consumption
	 * and moderate accuracy.
	 *
	 * 3.6 uA @1 Hz humidity, pressure and temperature. 0.1 uA in sleep mode.
	 */
	settings.osr_h = BME280_OVERSAMPLING_1X;
	settings.osr_p = BME280_OVERSAMPLING_16X;
	settings.osr_t = BME280_OVERSAMPLING_2X;
	settings.filter = BME280_FILTER_COEFF_16;
	settings.standby_time = BME280_STANDBY_TIME_1_MS;
	settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
	               BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

	dev = trb_bme280_create_i2c_dev(I2C_ADDRESS_BME280, settings);
	err = bme280_init(&dev);
	if (err != BME280_OK) {
		Serial.println(F("bme280_init()"));
		goto fail;
	}
	err = bme280_set_sensor_settings(settings_sel, &dev);
	if (err != 0) {
		Serial.println(F("bme280_set_sensor_settings()"));
		goto fail;
	}
	err = bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev);
	if (err != 0) {
		Serial.println(F("bme280_set_sensor_mode()"));
		goto fail;
	}
	err = 0;
fail:
	return err;
}

/* Initialize OLED display.
 *
 * Initialize OLED display, erase previous data, turn the display on, draws
 * boot message.
 */
void
display_init()
{
	display.init();
	display.clear();
	display.displayOff();
	display.normalDisplay();
	display.setFont(Roboto_Mono_10);
	display.displayOn();
	if (!display.setLogBuffer(MAX_N_LINE, MAX_LINE_LENGTH)) {
		/* malloc() failed */
		Serial.println("setLogBuffer()");
		halt();
	}
	logBootMessage("Starting.");
	display.display();
}

/*
 * Log a message to serial console and the OLED display.
 *
 * The given text will be displayed to console and display. The message will
 * be terminated with line feed. Messages are scrolled as more messages are
 * shown. As such, only useful upon boot, that is, before user interace is
 * created.
 */
void
logBootMessage(const String text)
{
	Serial.println(text);
	display.clear();
	display.println(text);
	display.drawLogBuffer(0, 0);
	display.display();
}

char *
float2string(float f)
{
	/* -ddddd.dd */
	/* 5 digits + 2 digits + 1 character for `-` + 1 character for `.` 9
	 * characters in total. the buffer needs another byte for NULL
	 * terminater. minimum buffer size is 10. */
	static char buffer[10];

	/* as it cannot handle more than 100000, return invalid strings, as in
	 * float */
	if (f >= 100000 || f <= -100000) {
		strlcpy(buffer, "-XXXXX.XX", sizeof(buffer));
	} else {
		dtostrf(f, 9, 2, buffer);
	}
	return buffer;
}

void
setup()
{
	int err;
	uint32_t time_wifi_connect_started;
	uint32_t duration_wifi_timeout = 30 * 1000;

	Serial.begin(115200);
	delay(3);
	Serial.println();
	Serial.println(__FILE__);

	Serial.println(F("initializing I2C"));
	err = i2c_init();
	if (err != 0) {
		halt();
	}

	Serial.println(F("initializing OLED display"));
	display_init();

	logBootMessage(F("initializing BME280"));
	err = bme280_init();
	if (err != 0) {
		logBootMessage("bme280_init");
		halt();
	}

	logBootMessage(F("Connecting to WiFi"));
	WiFi.begin("AP_NAME", "PASSWORD");
	time_wifi_connect_started = millis();
	while (WiFi.status() != WL_CONNECTED) {
		if ((millis() - time_wifi_connect_started) > duration_wifi_timeout) {
			logBootMessage("WiFi.begin timeout");
			halt();
		}
		delay(1000);
		Serial.print(".");
	}
	Serial.println();
	logBootMessage("Connected.");
	logBootMessage("IPv4: " + WiFi.localIP().toString());
	logBootMessage("GW: " + WiFi.gatewayIP().toString());
}

float get_temparature()
{
	float t = 11.1;
	return t;
}

float
get_humidiy()
{
	float h = 80.1;
	return h;
}

float
get_presure()
{
	float p = 1011.1;
	return p;
}

void
sleep()
{
	if (bme280_set_sensor_mode(BME280_SLEEP_MODE, &dev) != 0) {
		Serial.println(F("bme280_set_sensor_mode()"));
	}
	delay(1000);
}

void
loop()
{
	if (bme280_get_sensor_data(BME280_ALL, &data, &dev) != 0) {
		Serial.println(F("bme280_get_sensor_data()"));
		goto err;
	}
	Serial.print(F("T: "));
	Serial.print(float2string((float)data.temperature / 100));
	Serial.print(F(" degree "));
	Serial.print(F("F: "));
	Serial.print(float2string((float)data.humidity / 1024));
	Serial.print(F(" % "));
	Serial.print(F("P: "));
	Serial.print(float2string((float)data.pressure / 1000));
	Serial.print(F(" Pa"));
	Serial.println();
err:
	delay(1000);
}
