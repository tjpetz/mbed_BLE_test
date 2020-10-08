#include <mbed.h>
#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/services/BatteryService.h"

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
        _battery_service(ble, _battery_level), _adv_data_builder(_adv_buffer) {}

  void start() {
    _ble.gap().setEventHandler(this);
    _ble.init(this, &BatteryDemo::on_init_complete);
    _event_queue.call_every(500, this, &BatteryDemo::blink);
    _event_queue.call_every(1000, this, &BatteryDemo::update_sensor_value);
    _event_queue.call_every(15000, this, &BatteryDemo::status);
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
    ble::AdvertisingParameters adv_parameters(
        ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
        ble::adv_interval_t(ble::millisecond_t(1000)));

    _adv_data_builder.setFlags();
    _adv_data_builder.setLocalServiceList(mbed::make_Span(&_battery_uuid, 1));
    _adv_data_builder.setName(DEVICE_NAME);

    adv_parameters.setTxPower(-12);

    // ble_error_t error = _ble.gap().setAdvertisingParameters(
    //     ble::LEGACY_ADVERTISING_HANDLE, adv_parameters);
    ble_error_t error = _ble.gap().setAdvertisingParameters(
        ble::LEGACY_ADVERTISING_HANDLE,  
        ble::AdvertisingParameters(
          // ble::advertising_type_t::CONNECTABLE_NON_SCANNABLE_UNDIRECTED,
          // false
          ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
          true
          )
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
    }
  }

  void blink(void) { led1 = !led1; }

  void status(void) {
    // static int8_t power = -4;
    // power += 2;
    // if (power > 4)
    //   power = -4;

    // Serial.print("Setting Power to: ");
    // Serial.println(power);
    // _ble.gap().setTxPower(power);

    // const int8_t *p;
    // size_t count;

    // _ble.gap().getPermittedTxPowerValues(&p, &count);

    // if (count > 0) {
    //   Serial.print("count = ");
    //   Serial.println(count);
    //   for (int i = 0; i < count; i++) {
    //     Serial.print("power = ");
    //     Serial.println(p[i]);
    //   }
    // } else {
    //   Serial.println("Permitted powers didn't return anything!");
    // }

    // Gap::ConnectionParams_t connParams;
    // ble_error_t error = _ble.gap().getPreferredConnectionParams(&connParams);
    // if (error == BLE_ERROR_NONE) {
    //   Serial.println("Connection params:");
    //   Serial.println(connParams.minConnectionInterval);
    // } else {
    //   Serial.println("Error getting preferred connection params!");
    // }

    Serial.println("================================");

    Serial.print("Max AdvertisingSetNumber: ");
    Serial.println(_ble.gap().getMaxAdvertisingSetNumber());
    Serial.print("LE_ENCRYPTION: ");
    Serial.println(_ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_ENCRYPTION));
    Serial.print("LE_2M_PHY: ");
    Serial.println(_ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_2M_PHY));

  }

private:
  virtual void
  onDisconnectionComplete(const ble::DisconnectionCompleteEvent &) {
    Serial.println("Disconnecting");
    _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  }

  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &) {
    Serial.println("Connecting!");
  }

  virtual void onAdvertisingEnd(const ble::AdvertisingEndEvent &) {
    Serial.println("Restarting advertising");
    _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  }

  BLE &_ble;
  events::EventQueue &_event_queue;
  UUID _battery_uuid;
  uint8_t _battery_level;
  BatteryService _battery_service;
  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;
};

void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
  event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

void setup() {
  Serial.begin(115200);
  wait(5);

  Serial.println("Starting...");

  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(schedule_ble_events);

  BatteryDemo demo(ble, event_queue);
  demo.start();
}

void loop() {}
