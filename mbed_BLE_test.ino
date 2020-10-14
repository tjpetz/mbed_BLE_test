#include <mbed.h>
#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/services/BatteryService.h"
#include "ble/services/EnvironmentalService.h"
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>

static mbed::DigitalOut led1(LED1, 1);

const static char DEVICE_NAME[] = "BATTERY";

static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

void print_error(ble_error_t err, const char *msg) {
  Serial.print("Error: ");
  Serial.print(err);
  Serial.print(" -- ");
  Serial.println(msg);
}

class BatteryDemo : ble::Gap::EventHandler {
public:
  BatteryDemo(BLE &ble, events::EventQueue &event_queue)
      : _ble(ble), _event_queue(event_queue),
        _battery_uuid(GattService::UUID_BATTERY_SERVICE), _battery_level(50),
        _environmental_uuid(GattService::UUID_ENVIRONMENTAL_SERVICE),
        _temperature(-40), _humidity(0), _pressure(0),
        _battery_service(ble, _battery_level), 
        _environmental_service(ble),
        _adv_data_builder(_adv_buffer) {}

  void start() {
    _ble.gap().setEventHandler(this);
    _ble.init(this, &BatteryDemo::on_init_complete);
    _event_queue.call_every(500, this, &BatteryDemo::blink);
    _event_queue.call_every(1000, this, &BatteryDemo::update_sensor_value);
    _event_queue.dispatch_forever();
  }

private:
  void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
    if (params->error != BLE_ERROR_NONE) {
      print_error(params->error, "Ble initialization failed.");
      return;
    }

    Serial.println("Ble initialization complete!");

    start_advertising();
  }

  void start_advertising() {

    // setup the advertising data, defaults and add the battery service uuid.
    _adv_data_builder.setFlags();
    _adv_data_builder.setLocalServiceList(mbed::make_Span(&_environmental_uuid, 1));
    _adv_data_builder.setName(DEVICE_NAME);

    // Note, we force the Own Address type to public.  The default is random which
    // seems to prevent connections from macOS 10.15 (and possibly other version).
    ble_error_t error = _ble.gap().setAdvertisingParameters(
        ble::LEGACY_ADVERTISING_HANDLE,  
        ble::AdvertisingParameters()
          .setOwnAddressType(ble::own_address_type_t::PUBLIC)
          .setTxPower(8)
          );

    if (error) {
      print_error(error, "_ble.gap().setAdvertisingParameters() failed");
      return;
    }

    error = _ble.gap().setAdvertisingPayload(
        ble::LEGACY_ADVERTISING_HANDLE, _adv_data_builder.getAdvertisingData());

    if (error) {
      print_error(error, "_ble.gap().setAdvertisingPayload() failed");
      return;
    }

    error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      print_error(error, "_ble.gap().startAdvertising() failed");
      return;
    }
  }

  void update_sensor_value() {
   if (_ble.gap().getState().connected) {
      _battery_level++;
      if (_battery_level > 100)
        _battery_level = 20;

      _battery_service.updateBatteryLevel(_battery_level);

      _environmental_service.updateTemperature(HTS.readTemperature() * 100);
      _environmental_service.updateHumidity(HTS.readHumidity() * 100);
      _environmental_service.updatePressure(BARO.readPressure() * 10000);
   }
  }

  void blink(void) { led1 = !led1; }

private:
  virtual void
  onDisconnectionComplete(const ble::DisconnectionCompleteEvent & a) {
    Serial.println("Disconnecting");
    _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  }

  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent & a) {
    Serial.println("Connecting!");
  }

  virtual void onAdvertisingEnd(const ble::AdvertisingEndEvent & a) {
    Serial.println("Restarting advertising");
    _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  }

  virtual void onUpdateConnectionParametersRequest(const ble::UpdateConnectionParametersRequestEvent & a) {
    Serial.println("Received request to update connection parameters!\n");
  }

  BLE &_ble;
  events::EventQueue &_event_queue;
  UUID _battery_uuid;
  UUID _environmental_uuid;
  uint8_t _battery_level;
  int16_t _temperature;
  uint16_t _humidity;
  uint32_t _pressure;

  BatteryService _battery_service;
  EnvironmentalService _environmental_service;

  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;
};

void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
  event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

void setup() {
  Serial.begin(115200);
  delay(5);

  Serial.println("Starting...");

  HTS.begin();
  BARO.begin();

  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(schedule_ble_events);

  BatteryDemo demo(ble, event_queue);
  demo.start();
}

void loop() {}
