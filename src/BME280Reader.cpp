#include "BME280Reader.h"

int
BME280Reader::begin(uint8_t i2c_address, struct bme280_settings config, uint32_t interval_millis)
{
	int err;

	intervalMillis = interval_millis;
	dev = trb_bme280_create_i2c_dev(i2c_address, config);
	if ((err = bme280_init(&dev)) != 0) {
		goto err;
	}
	err = 0;
err:
	return err;
}

int
BME280Reader::setConfig(uint8_t settings)
{
	int err;
	if ((err = bme280_set_sensor_settings(settings, &dev)) != 0) {
		goto err;
	}
	err = 0;
err:
	return err;
}

int
BME280Reader::setMode(uint8_t mode)
{
	int err;
	if ((err = bme280_set_sensor_mode(mode, &dev)) != 0) {
		goto err;
	}
	err = 0;
err:
	return err;
}

int
BME280Reader::read(bme280_data *data, const uint32_t updateTimeMillis)
{
	int err;
	if (updateTimeMillis - lastUpdatedMillis > intervalMillis ||
	    lastUpdatedMillis == 0) {
		err = bme280_get_sensor_data(BME280_ALL, data, &dev);
		lastUpdatedMillis = updateTimeMillis;
		if (err != 0) {
			goto err;
		}
		Serial.print(F("T: "));
		Serial.print(String((float)data->temperature / 100));
		Serial.print(F(" degree "));
		Serial.print(F("F: "));
		Serial.print(String((float)data->humidity / 1024));
		Serial.print(F(" % "));
		Serial.print(F("P: "));
		Serial.print(String((float)data->pressure / 1000));
		Serial.print(F(" Pa"));
		Serial.println();
	}
	err = 0;
err:
	return err;
}
