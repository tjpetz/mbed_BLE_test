# mbed_BLE_test

This is an extension of the mbedOS battery example.  It includes the environmental service and uses
the sensor on the Arduino Nano 33 BLE Sense board.

## Public and Random BLE address on macOS

By default the mbedOS library advertises with Random Addresses.  This is different than the default
for the Arduino BLE library which uses the Public Address to advertise.  This difference caused
connectivity problems with macOS clients.  An iOS 14 (either iPhone or iPad) could connect but
the similar client on a macOS 10.15 (Catalina) machine would scan but not connect.  Using the
PacketLogger application include in the XCode Extra Tools, I was able to observe the difference
between connecting to a public and random address.  Forcing the mbedOS peripheral to advertise
with the public address resolved the problem.

```
    ble_error_t error = _ble.gap().setAdvertisingParameters(
        ble::LEGACY_ADVERTISING_HANDLE,  
        ble::AdvertisingParameters()
          .setOwnAddressType(ble::own_address_type_t::PUBLIC)
          .setTxPower(8)
          );
```
