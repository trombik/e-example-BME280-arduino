#if !defined(_ThingSpeakUpdater_h)
#define _ThingSpeakUpdater_h

#include <TRB_BME280.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>

/* Fields in the channel. Create a channel on ThingSpeak accordingly.
 * By default, a channel has one field. Add additional two field.
 */
#if !defined(THINGSPEAK_API_FIELD_TEMPERATURE)
#define THINGSPEAK_API_FIELD_TEMPERATURE	(1)
#endif
#if !defined(THINGSPEAK_API_FIELD_HUMIDITY)
#define THINGSPEAK_API_FIELD_HUMIDITY	(2)
#endif
#if !defined(THINGSPEAK_API_FIELD_PRESSURE)
#define THINGSPEAK_API_FIELD_PRESSURE	(3)
#endif

class ThingSpeakUpdater
{
	uint32_t intervalMillis;
	uint32_t lastUpdatedMillis;
	uint32_t channelNumber;
	const char *apiKey;
	WiFiClient client;

	public:
	ThingSpeakUpdater()
	{
		apiKey = NULL;
		intervalMillis = 300 * 1000;
		channelNumber = 0;
		lastUpdatedMillis = 0;
	}

	/* \brief initialize ThingSpeakUpdater object
	 *
	 * \param api_key ThingSpeak Write API key.
	 * \param channel_id ThingSpeak channel ID to submit the data.
	 * \param interval_sec Update interval in second.
	 */
	void begin(const char *api_key, uint32_t channel_id, uint32_t interval_sec)
	{
		apiKey = api_key;
		intervalMillis = interval_sec * 1000;
		channelNumber = channel_id;
		ThingSpeak.begin(client);
	}

	/* \brief send values to thingspeak channel.
	 *
	 * Send values to thingspeak at interval_sec. If an error occurs, it does
	 * not retry.
	 *
	 * \return int Zero if no update has been made, 200 if successfullly
	 * updated. other values from ThingSpeak.h
	 */
	int update(const bme280_data data, const uint32_t updateTimeMillis)
	{
		int err = 0;
		if (updateTimeMillis - lastUpdatedMillis > intervalMillis ||
	        lastUpdatedMillis == 0) {
			ThingSpeak.setField(
				THINGSPEAK_API_FIELD_TEMPERATURE,
				(float)data.temperature / 100
			);
			ThingSpeak.setField(
				THINGSPEAK_API_FIELD_HUMIDITY,
				(float)data.humidity / 1024
			);
			ThingSpeak.setField(
				THINGSPEAK_API_FIELD_PRESSURE,
				(float)data.pressure / 1000
			);
			err = ThingSpeak.writeFields(channelNumber, apiKey);

			/* regardless of the result of writeFields(), update the counter so that
			 * it does not repeatedly try to submit values when the failure
			 * persists. otherwise, the UI (the display) gets interrupted often,
			 * too.
			 */
			lastUpdatedMillis = updateTimeMillis;
			if (err != OK_SUCCESS) {
				goto err;
			}

		}
err:
		return err;
	}
};
#endif // _ThingSpeakUpdater_h
