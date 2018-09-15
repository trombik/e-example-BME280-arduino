# e-example-BME280-arduino

[![Build Status](https://travis-ci.com/trombik/e-example-BME280-arduino.svg?branch=master)](https://travis-ci.com/trombik/e-example-BME280-arduino)

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

## Using the device

The device needs WiFi network access to submit sensor values to ThingSpeak. It
is required to configure WiFi network.

When the device initially starts, it runs WiFi access point. SSID, WiFi
password, and the IPv4 address of the device are shown in the display.

Connect to the WiFi access point. Open a browser, type the IP address shown in
the display to URL bar. Recent smartphone automatically tells you that the
WiFi network needs sign-in when the connection has been established.

Select a WiFi network and set the WiFi password. The device will restart.

## Development

### Requirements

Optional requirements below ensure that the code passes CI checks by
`travisci`.

* ruby
* [Bundler](https://bundler.io/)
* [cppcheck](http://cppcheck.sourceforge.net/)
* [astyle](http://astyle.sourceforge.net/)

### Preparing

Install gems.

```
bundle install --path=vendor
```

Run `guard`.

```
bundle exec guard
```

`guard` watches files under `src/`, and rebuilds sources whenever the files
change.

Set environment variables.

```
export THINGSPEAK_API_CHANNEL=123456
export THINGSPEAK_API_KEY=YOUR_API_KEY
```

These values must match your _Channel ID_ and _Write API Key_ shown in the
ThingSpeak channel.

### Developing

Develop the code. Debug logs are shown in serial console at 115200 baud rate.
Before commit, run `rake`.

```
bundle exec rake
```

The default `rake` task also checks code style, and formats source files.
