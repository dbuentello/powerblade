/*
 * PowerBlade BLE application
 */

// Standard Libraries
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Nordic Libraries
#include "ble.h"
#include "ble_advdata.h"
#include "app_timer.h"
#include "app_util_platform.h"
#include "nrf_uart.h"
#include "nrf_drv_common.h"

// Platform, Peripherals, Devices, & Services
#include "ble_config.h"
#include "uart.h"
#include "powerblade.h"
#include "led.h"
#include "simple_ble.h"
#include "simple_adv.h"
#include "eddystone.h"
#include "checksum.h"


/**************************************************
 * Function Protoypes
 **************************************************/
void start_eddystone_adv(void);
void init_adv_data(void);
void start_manufdata_adv(void);

void UART0_IRQHandler(void);
void uart_rx_handler(void);
void process_rx_packet(uint16_t packet_len);
void uart_tx_handler(void);
void uart_send(uint8_t* data, uint16_t len);

void services_init(void);
void ble_evt_write (ble_evt_t* p_ble_evt);

void timers_init(void);
int main(void);


/**************************************************
 * Define and Globals
 **************************************************/

static ble_app_t app;

#define PHYSWEB_URL "goo.gl/jEKPu9"

// timer configuration
#define TIMER_PRESCALER     0
#define TIMER_OP_QUEUE_SIZE 4
APP_TIMER_DEF(enable_uart_timer);
APP_TIMER_DEF(start_eddystone_timer);
APP_TIMER_DEF(start_manufdata_timer);
#define EDDYSTONE_ADV_DURATION APP_TIMER_TICKS(200, TIMER_PRESCALER)
#define MANUFDATA_ADV_DURATION APP_TIMER_TICKS(800, TIMER_PRESCALER)
#define UART_SLEEP_DURATION    APP_TIMER_TICKS(940, TIMER_PRESCALER)

// advertisement data collected from UART
#define ADV_DATA_MAX_LEN 24 // maximum manufacturer specific advertisement data size
static uint8_t powerblade_adv_data[ADV_DATA_MAX_LEN];
static uint8_t powerblade_adv_data_len = 0;

// uart buffers
#define RX_DATA_MAX_LEN 50
static uint8_t rx_data[RX_DATA_MAX_LEN];
static uint8_t* tx_data;
static uint16_t tx_data_len = 0;

/**************************************************
 * Advertisements
 **************************************************/

void start_eddystone_adv (void) {
    uint32_t err_code;

    // Advertise physical web address for PowerBlade summon app
    eddystone_adv(PHYSWEB_URL, NULL);

    err_code = app_timer_start(start_manufdata_timer, EDDYSTONE_ADV_DURATION, NULL);
    APP_ERROR_CHECK(err_code);
}

void init_adv_data (void) {
    // Default values, helpful for debugging
    powerblade_adv_data[0] = 0x01; // Version
    
    powerblade_adv_data[1] = 0x01;
    powerblade_adv_data[2] = 0x01;
    powerblade_adv_data[3] = 0x01;
    powerblade_adv_data[4] = 0x01; // Sequence

    powerblade_adv_data[5] = 0x42;
    powerblade_adv_data[6] = 0x4A; // P_Scale

    powerblade_adv_data[7] = 0x7B; // V_Scale

    powerblade_adv_data[8] = 0x09; // WH_Scale

    powerblade_adv_data[9] = 0x31; // V_RMS

    powerblade_adv_data[10] = 0x08;
    powerblade_adv_data[11] = 0x02; // True Power

    powerblade_adv_data[12] = 0x0A;
    powerblade_adv_data[13] = 0x1A; // Apparent Power

    powerblade_adv_data[14] = 0x00;
    powerblade_adv_data[15] = 0x00;
    powerblade_adv_data[16] = 0x01;
    powerblade_adv_data[17] = 0x0D; // Watt Hours

    powerblade_adv_data[18] = 0x00; // Flags
    powerblade_adv_data_len = 19;
}

void start_manufdata_adv (void) {
    uint32_t err_code;

    // Advertise PowerBlade data payload as manufacturer specific data
    ble_advdata_manuf_data_t manuf_specific_data;
    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = powerblade_adv_data;
    manuf_specific_data.data.size   = powerblade_adv_data_len;
    simple_adv_manuf_data(&manuf_specific_data);

    err_code = app_timer_start(start_eddystone_timer, MANUFDATA_ADV_DURATION, NULL);
    APP_ERROR_CHECK(err_code);
}

/**************************************************
 * UART
 **************************************************/

void UART0_IRQHandler (void) {
    if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY)) {
        uart_rx_handler();
    } else if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY)) {
        uart_tx_handler();
    }
}

void uart_rx_handler (void) {
    static uint16_t packet_len = 0;
    static uint16_t rx_index = 0;

    // data is available
    nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);
    rx_data[rx_index] = nrf_uart_rxd_get(NRF_UART0);
    rx_index++;

    // check if we have received the entire packet
    //  This can't occur until we have length, adv length, and checksum
    if (rx_index >= 4) {

        // parse out expected packet length
        if (packet_len == 0) {
            packet_len = (rx_data[0] << 8 | rx_data[1]);
        }

        // process packet if we have all of it
        if (rx_index >= packet_len || rx_index >= RX_DATA_MAX_LEN) {
            process_rx_packet(packet_len);
            packet_len = 0;
            rx_index = 0;
        }
    }
}

void process_rx_packet(uint16_t packet_len) {

    // turn off UART until next window
    uart_rx_disable();
    app_timer_start(enable_uart_timer, UART_SLEEP_DURATION, NULL);

    // check CRC
    if (additive_checksum(rx_data, packet_len-1) == rx_data[packet_len-1]) {

        // check validity of advertisement length
        uint8_t adv_len = rx_data[2];
        if (4+adv_len <= packet_len) {

            // limit to valid advertisement length
            if (adv_len > ADV_DATA_MAX_LEN) {
                adv_len = ADV_DATA_MAX_LEN;
            }

            // update advertisement
            //NOTE: this is safe to call no matter where in the
            //  Eddystone/Manuf Data we are. If called during Manuf Data,
            //  nothing changes (second call to timer_start does nothing).
            //  If called during Eddystone, timing is screwed up, but it'll fix
            //  itself within one cycle
            powerblade_adv_data_len = adv_len;
            memcpy(powerblade_adv_data, &(rx_data[3]), adv_len);
            start_manufdata_adv();
        }

        // handle additional UART data
        // Do nothing for now
    }
}

void uart_tx_handler (void) {
    static uint16_t tx_index = 0;

    // clear event
    nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);

    // check if there is more data to send
    if (tx_index < tx_data_len) {
        nrf_uart_txd_set(NRF_UART0, tx_data[tx_index]);
        tx_index++;
    } else {
        tx_index = 0;
        uart_tx_disable();
    }
}

void uart_send (uint8_t* data, uint16_t len) {
    // setup data
    tx_data = data;
    tx_data_len = len;

    // begin sending data
    uart_tx_enable();
    uart_tx_handler();
}

/**************************************************
 * Services
 **************************************************/

// Randomly generated UUID
const ble_uuid128_t calibrate_uuid128 = {
    {0x99, 0xf9, 0xac, 0xe5, 0x57, 0xb9, 0x43, 0xec,
     0x88, 0xf8, 0x88, 0xb9, 0x4d, 0xa1, 0x80, 0x50}
};
ble_uuid_t calibrate_uuid;

#define CALIBRATE_SHORT_UUID            0x57B9
#define CALIBRATE_CHAR_BEGIN_SHORT_UUID 0x57BA
#define CALIBRATE_CHAR_WATTS_SHORT_UUID 0x57BB
#define CALIBRATE_CHAR_NEXT_SHORT_UUID  0x57BC
#define CALIBRATE_CHAR_DONE_SHORT_UUID  0x57BD

void services_init (void) {

    // Add main app upload service
    app.calibrate_service_handle = simple_ble_add_service(&calibrate_uuid128,
                                                &calibrate_uuid,
                                                CALIBRATE_SHORT_UUID);

    // Add the characteristic to write current watt setting
    app.begin_calibration = false;
    simple_ble_add_characteristic(0, 1, 0, // read, write, notify
                                  calibrate_uuid.type,
                                  CALIBRATE_CHAR_BEGIN_SHORT_UUID,
                                  1, (uint8_t*)&app.begin_calibration,
                                  app.calibrate_service_handle,
                                  &app.calibrate_char_begin_handle);

    // Add the characteristic to write current watt setting
    app.ground_truth_watts = -1;
    simple_ble_add_characteristic(0, 1, 0, // read, write, notify
                                  calibrate_uuid.type,
                                  CALIBRATE_CHAR_WATTS_SHORT_UUID,
                                  2, (uint8_t*)&app.ground_truth_watts,
                                  app.calibrate_service_handle,
                                  &app.calibrate_char_watts_handle);

    // Add the characteristic to write current watt setting
    app.next_wattage = false;
    simple_ble_add_characteristic(1, 0, 1, // read, write, notify
                                  calibrate_uuid.type,
                                  CALIBRATE_CHAR_NEXT_SHORT_UUID,
                                  1, (uint8_t*)&app.next_wattage,
                                  app.calibrate_service_handle,
                                  &app.calibrate_char_next_handle);

    // Add the characteristic to write current watt setting
    app.done_calibrating = false;
    simple_ble_add_characteristic(1, 0, 1, // read, write, notify
                                  calibrate_uuid.type,
                                  CALIBRATE_CHAR_DONE_SHORT_UUID,
                                  1, (uint8_t*)&app.done_calibrating,
                                  app.calibrate_service_handle,
                                  &app.calibrate_char_done_handle);
}

static bool calibration_mode = false;
static uint8_t tx_buffer[10];

void ble_evt_write (ble_evt_t* p_ble_evt) {
    ble_gatts_evt_write_t* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == app.calibrate_char_begin_handle.value_handle) {
        // copy over data, single byte
        if (app.begin_calibration) {
            calibration_mode = true;

            // send start calibration message to MSP430
            uint16_t length = 2+2+1+1; // length, short UUID, value, checksum
            tx_buffer[0] = (length >> 8);
            tx_buffer[1] = (length & 0xFF);
            tx_buffer[2] = (CALIBRATE_CHAR_BEGIN_SHORT_UUID >> 8);
            tx_buffer[3] = (CALIBRATE_CHAR_BEGIN_SHORT_UUID & 0xFF);
            tx_buffer[4] = (uint8_t)true;
            tx_buffer[5] = additive_checksum(tx_buffer, length-1);
            uart_send(tx_buffer, length);
        }

    } else if (p_evt_write->handle == app.calibrate_char_watts_handle.value_handle) {
        // correct upper bits if user only wrote a single byte
        //  It's reasonable to assume the user will only write unsigned data
        if (p_evt_write->len == 1) {
            app.ground_truth_watts = (0x0000 | (uint8_t)p_evt_write->data[0]);
        }

        if (calibration_mode) {
            // send ground truth wattage to MSP430
            uint16_t length = 2+2+2+1; // length, short UUID, value, checksum
            tx_buffer[0] = (length >> 8);
            tx_buffer[1] = (length & 0xFF);
            tx_buffer[2] = (CALIBRATE_CHAR_BEGIN_SHORT_UUID >> 8);
            tx_buffer[3] = (CALIBRATE_CHAR_BEGIN_SHORT_UUID & 0xFF);
            tx_buffer[4] = (app.ground_truth_watts >> 8);
            tx_buffer[5] = (app.ground_truth_watts & 0xFF);
            tx_buffer[6] = additive_checksum(tx_buffer, length-1);
            uart_send(tx_buffer, length);
        }
    }
}


/**************************************************
 * Initialization
 **************************************************/

void timers_init (void) {
    uint32_t err_code;

    APP_TIMER_INIT(TIMER_PRESCALER, TIMER_OP_QUEUE_SIZE, false);

    err_code = app_timer_create(&enable_uart_timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)uart_rx_enable);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&start_eddystone_timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)start_eddystone_adv);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&start_manufdata_timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)start_manufdata_adv);
    APP_ERROR_CHECK(err_code);
}


int main(void) {
    // Initialization
    app.simple_ble_app = simple_ble_init(&ble_config);
    uart_init();
    timers_init();
    init_adv_data();

    // Initialization complete
    start_manufdata_adv();
    uart_rx_enable();

    while (1) {
        power_manage();
    }
}
