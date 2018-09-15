#if !defined(_BME280Reader_h)
#define _BME280Reader_h

#include "Arduino.h"
#include <TRB_BME280.h>

class BME280Reader
{
public:
	/* \brief initialize the updater
	 *
	 * \param i2c_address I2C address of BME280
	 * \param config struct bme280_settings
	 * \param interval_millis Interval at which the updater reads values from
	 *        the sensor
	 * \return Zero on success, non-zero on failure
	 */
	int begin(uint8_t i2c_address, struct bme280_settings config, uint32_t interval_millis);

	/* \brief call bme280_set_sensor_settings()
	 * \param settings bme280_settings
	 * \return Zero on success, non-zero on failure
	 */
	int setConfig(uint8_t settings);

	/* \brief call bme280_set_sensor_mode
	 *
	 * \param mode BME280 mode
	 * \return Zero on success, non-zero on failure
	 */
	int setMode(uint8_t mode);

	/* \brief Reads sensor values from BME280 at intervals, prints values to
	 *        serial console.
	 * \param data Variable to writes the values to
	 * \param updateTimeMillis millis() value on which the interval is
	 *        calculated.
	 */
	int read(bme280_data *data, const uint32_t updateTimeMillis);

private:
	struct bme280_dev dev;
	uint32_t intervalMillis;
	uint32_t lastUpdatedMillis = 0;
};

#endif
