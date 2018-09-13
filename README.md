# e-example-BME280-arduino

An example project using popular environmental sensor,
[BME280](https://www.bosch-sensortec.com/bst/products/all_products/bme280).
The device reads values from BME280, such as temperature, humidity, and
pressure. The values are shown in attached display. Optionally, sends values
to [ThingSpeak](https://thingspeak.com). It is designed as a device connected
to 5V power source, not battery.

## Building

### Requirements

* [platformio](https://platformio.org/)

```
pio run
```

## Development

### Requirements

* ruby
* [Bundler](https://bundler.io/)
* [cppcheck](http://cppcheck.sourceforge.net/)
* [astyle](http://astyle.sourceforge.net/)


Set environment variables

```
export THINGSPEAK_API_CHANNEL=123456
export THINGSPEAK_API_KEY=YOUR_API_KEY
```

```
bundle install --path=vendor
```

Run `guard`

```
bundle exec guard
```
