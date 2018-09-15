#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

/* ThingSpeak API updater */
#include "ThingSpeakUpdater.h"
#include "BME280Reader.h"

/* for OLED display */
#include <OLEDDisplayUi.h>
#include <SSD1306Brzo.h>

/* for custom font, Roboto_Mono_6 */
#include "Roboto_Mono_10.h"
#include "DejaVu_Serif_27.h"

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

/* key and channel number of ThingSpeak API, mandatory */
#if !defined(THINGSPEAK_API_KEY)
#error THINGSPEAK_API_KEY must be defined
#endif
#if !defined(THINGSPEAK_API_CHANNEL)
#error THINGSPEAK_API_CHANNEL must be defined
#endif

/* bme280 device instance */
static struct bme280_dev dev;
/* data structure returned by the bme280 driver */
static struct bme280_data data;

/* OLEDDisplay instance */
SSD1306Brzo display(I2C_ADDRESS_SSD1306, SDA, SCL);
/* UI instance */
OLEDDisplayUi ui(&display);

/* BME280 reader that reads values from the device at intervals */
BME280Reader reader;

/* ThingSpeak worker that updates sensor values */
ThingSpeakUpdater ThingSpeakUpdater;

/* frames of ui */
void frame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void frame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
FrameCallback frames[] = { frame1, frame2 };
uint8_t n_frames = 2;

/* password of WiFiManager AP, not the network the device will connect to.
 * max length of 8 + null character
 */
char password[9];

/* macros for stringizing a macro.
 * https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
 */
#define str(s) #s
#define xstr(s) str(s)
const char thingspeak_api_key[] = xstr(THINGSPEAK_API_KEY);
const uint32_t thingspeak_api_channel = THINGSPEAK_API_CHANNEL;

/* pin number to enable/disable thingspeak support. if this pin is high,
 * thingspeak is enabled, and sensor values will be sent to thingspeak.
 */
#define THINGSPEAK_ENABLE_PIN	D5

/* update interval in sec. for free tier, must be more than 30 (two updates
 * per minute). Default is 300
 */
#if !defined(THINGSPEAK_API_INTERVAL)
#define THINGSPEAK_API_INTERVAL	(300)
#endif
const uint16_t interval_thingspeak_update_sec = THINGSPEAK_API_INTERVAL;

/* interval in sec to read values from BME280. */
#if !defined(READ_INTERVAL)
#define READ_INTERVAL	(1)
#endif
uint16_t interval_read_sec = READ_INTERVAL;

/* last updated time of ThingSpeak fileds in millis */
uint32_t last_thingspeak_updated_millis = 0;

/* last time in millis when values are read from BME280 */
uint32_t last_read_millis = 0;

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
init_bme280()
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

	if ((err = reader.begin(I2C_ADDRESS_BME280, settings, interval_read_sec * 1000)) != 0) {
		Serial.print(F("reader.begin()"));
		goto fail;
	}
	if ((err = reader.setConfig(settings_sel)) != 0) {
		Serial.print(F("reader.setConfig()"));
		goto fail;
	}
	if ((err = reader.setMode(BME280_NORMAL_MODE)) != 0) {
		Serial.println(F("reader.setMode()"));
		goto fail;
	}
	err = 0;
fail:
	if (err != 0) {
		Serial.println(err);
	}
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
		Serial.println(F("setLogBuffer()"));
		halt();
	}
	logBootMessage(F("Starting."));
	display.display();
}

void
init_ui()
{
	ui.setTargetFPS(10);
	ui.enableAutoTransition();
	ui.setTimePerFrame(3 * 1000);
	ui.setTimePerTransition(500);
	ui.disableAllIndicators();
	ui.setFrames(frames, n_frames);
	ui.init();
}

void
frame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
	char buf[3 + 2 + 1]; // (-)\d\d 'C
	uint8_t hight_text = DejaVu_Serif_27[1];
	display->setFont(DejaVu_Serif_27);
	display->setTextAlignment(TEXT_ALIGN_RIGHT);

	snprintf(buf, sizeof(buf), "%-2d %c", data.temperature / 100, 67);
	display->drawString(128 + x, 0 + y, buf);

	snprintf(buf, sizeof(buf), "%2d %%", data.humidity / 1024);
	display->drawString(128 + x, hight_text + y, buf);
}

void
frame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
	display->setFont(DejaVu_Serif_27);
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawString(64 + x, 0 + y, "ESP8266");
}

/*
 * \brief returns if thingspeak is enabled or not
 *
 * A pin is connected to a SPDT switch, a Single Pole Double Throw switch,
 * commonly known as `slide switch`
 *
 * Vcc --------o
 *              \
 *               `o---- COM --> micro controller's digital pin
 *
 * GND --------o    a SPDT switch
 *
 * The common is connected to one of pins of micro controller. Others are to
 * Vcc and GND. THINGSPEAK_ENABLE_PIN must be pulled up by
 * `pinMode(THINGSPEAK_ENABLE_PIN, INPUT_PULLUP)`.
 *
 * \return bool
 */
bool
is_thingspeak_enabled()
{
	return digitalRead(THINGSPEAK_ENABLE_PIN) == HIGH ? true : false;
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
	display.setFont(Roboto_Mono_10);
	display.clear();
	display.println(text);
	display.drawLogBuffer(0, 0);
	display.display();
}

/* A callback to show WiFi AP information on the display.
 *
 * called when AP has started (after wifiManager.autoConnect()).
 */
void
callbackWiFiManager(WiFiManager *manager)
{
	logBootMessage(F("AP started"));
	logBootMessage("SSID: " + manager->getConfigPortalSSID());
	logBootMessage("IP: " + WiFi.softAPIP().toString());
	logBootMessage("PW: " + String(password));
}

void
setup()
{
	int err;

	/* SSID. prefixed with "ESP", followed by eight digit chip ID. */
	char ssid[3 + 8 + 1];

	/* WiFi Connection manager, runs web configuration portal */
	WiFiManager wifiManager;

	Serial.begin(115200);
	delay(3);
	Serial.println();
	Serial.println(__FILE__);

	/* seed random(). the value is fixed during development.
	 * if in production, float A0 pin so that noise generates random seed
	 */
#if defined(RELEASE_BUILD)
	randomSeed(analogRead(0));
#else
	randomSeed(1);
#endif
	snprintf(ssid, sizeof(ssid), "ESP%i", ESP.getChipId());
	snprintf(password, sizeof(password), "%li", random(10000000, 99999999));

	Serial.println(F("initializing I2C"));
	err = i2c_init();
	if (err != 0) {
		halt();
	}

	Serial.println(F("initializing OLED display"));
	init_ui();
	display_init();

	logBootMessage(F("initializing BME280"));
	err = init_bme280();
	if (err != 0) {
		logBootMessage(F("bme280_init"));
		halt();
	}

	logBootMessage(F("Connecting to WiFi"));

	/* set a callback function when AP mode starts */
	wifiManager.setAPCallback(callbackWiFiManager);

	/* try to connect to previous network if found in memory. run
	 * configuration portal if not found, or if the attempt has failed.
	 */
	if (!wifiManager.autoConnect(ssid, password)) {
		/* either connection failed or timed out */
		logBootMessage(F("autoConnect()"));
		halt();
	}
	logBootMessage(F("Connected."));
	logBootMessage("IPv4: " + WiFi.localIP().toString());
	logBootMessage("GW: " + WiFi.gatewayIP().toString());
	pinMode(THINGSPEAK_ENABLE_PIN, INPUT_PULLUP);
	ThingSpeakUpdater.begin(
	    thingspeak_api_key,
	    thingspeak_api_channel,
	    interval_thingspeak_update_sec
	);
}

void
loop()
{
	int err = 0;
	uint32_t current_millis;

	current_millis = millis();
	if (ui.update() <= 0) {
		goto do_nothing;
	}
	if ((err = reader.read(&data, current_millis)) != 0) {
		goto err;
	}
	if (is_thingspeak_enabled()) {
		err = ThingSpeakUpdater.update(data, current_millis);
		if (err != 200 && err != 0) {
			Serial.print(F("ThingSpeakUpdater.update()"));
			goto err;
		}
	}
	err = 0;
err:
	if (err != 0) {
		Serial.println(err);
	}
do_nothing:
	yield();
}
