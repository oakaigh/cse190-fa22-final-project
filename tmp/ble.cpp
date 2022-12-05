//-------------------------------------------------------------------------------
//  TinyCircuits ST BLE TinyShield UART Example Sketch
//  Last Updated 2 March 2016
//
//  This demo sets up the BlueNRG-MS chipset of the ST BLE module for compatiblity 
//  with Nordic's virtual UART connection, and can pass data between the Arduino
//  serial monitor and Nordic nRF UART V2.0 app or another compatible BLE
//  terminal. This example is written specifically to be fairly code compatible
//  with the Nordic NRF8001 example, with a replacement UART.ino file with
//  'aci_loop' and 'BLEsetup' functions to allow easy replacement. 
//
//  Written by Ben Rose, TinyCircuits http://tinycircuits.com 
//  (who has no idea what they were doing)
//  customized by Aaron Mega Corp
//
//-------------------------------------------------------------------------------


#include <stdint.h>
#include <stdlib.h>
//#include "STBLE/src/STBLE.h"
#include "ASTBLE/STBLE.h"
#include "ble.h"

#if BLE_DEBUG
#include <stdio.h>
char sprintbuff[100];
#define PRINTF(...) {sprintf(sprintbuff,__VA_ARGS__);SerialUSB.print(sprintbuff);}
#else
#define PRINTF(...)
#endif

volatile uint8_t set_connectable = 1;
uint16_t connection_handle = 0;

//uint8_t ble_connection_state = false;
int connected = FALSE;

uint8_t ble_rx_buffer[21];
uint8_t ble_rx_buffer_len = 0;

#define  ADV_INTERVAL_MIN_MS  50
#define  ADV_INTERVAL_MAX_MS  100

uint16_t UART_Serv_Handle, UART_TX_Char_Handle, UART_RX_Char_Handle;



uint8_t Add_UART_Service(void)
{
  const constexpr uint8_t 
    uuid_uart_service[] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E},
    uuid_tx_char[]      = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E},
    uuid_rx_char[]      = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E}
  ;

  tBleStatus ret;
  uint8_t uuid[16];

  ret = aci_gatt_add_serv(UUID_TYPE_128,  uuid_uart_service, PRIMARY_SERVICE, 7, &UART_Serv_Handle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  ret = aci_gatt_add_char(UART_Serv_Handle, UUID_TYPE_128, uuid_tx_char, 20, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           16, 1, &UART_TX_Char_Handle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  ret = aci_gatt_add_char(UART_Serv_Handle, UUID_TYPE_128, uuid_rx_char, 20, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE, 0,
                           16, 1, &UART_RX_Char_Handle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  return BLE_STATUS_SUCCESS;

fail:
  PRINTF("Error while adding UART service.\n");
  return BLE_STATUS_ERROR ;

}


enum discoverable_mode_e {
  FULL, LIMITED, NONE
};


volatile bool discoverable = false;
void set_discoverable(
  std::size_t intvl_min_ms = ADV_INTERVAL_MIN_MS,
  std::size_t intvl_max_ms = ADV_INTERVAL_MAX_MS
)
{
  // already discoverable
  if (discoverable)
    return;

  tBleStatus ret;

  // TODO
  const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'C', 'S', 'E', '1', '9', '0', 'A'};

  hci_le_set_scan_resp_data(0, NULL);
  PRINTF("Enter Discoverable Mode.\n");

  ret = aci_gap_set_discoverable(
    ADV_IND,
    (intvl_min_ms * 1000) / 625, (intvl_max_ms * 1000) / 625,
    PUBLIC_ADDR, NO_WHITE_LIST_USE,
    sizeof(local_name), local_name, 
    0, NULL, 
    0, 0
  );
  if (ret != BLE_STATUS_SUCCESS)
    PRINTF("%d\n", (uint8_t)ret);

}


int ble_init(struct ble_conf conf) {
  int ret = 0;

  HCI_Init();
  /* Init SPI interface */
  BNRG_SPI_Init();
  /* Reset BlueNRG/BlueNRG-MS SPI interface */
  BlueNRG_RST();

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, conf.bdaddr);

  if (ret) {
    PRINTF("Setting BD_ADDR failed.\n");
    return ret;
  }

  ret = aci_gatt_init();

  if (ret) {
    PRINTF("GATT_Init failed.\n");
    return ret;
  }

  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);

  if (ret) {
    PRINTF("GAP_Init failed.\n");
    return ret;
  }

  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(conf.name), (uint8_t *)conf.name);

  if (ret) {
    PRINTF("aci_gatt_update_char_value failed.\n");
    return ret;
  }

  PRINTF("BLE Stack Initialized.\n");
  

  ret = Add_UART_Service();

  if (ret == BLE_STATUS_SUCCESS) {
    PRINTF("UART service added successfully.\n");
  } else {
    PRINTF("Error while adding UART service.\n");
    return ret;
  }

  /* +4 dBm output power */
  ret = aci_hal_set_tx_power_level(true, 3);
  //ret = aci_hal_set_tx_power_level(1, 6);

  set_discoverable();

  return ret;
}


// TODO
// NOTE level: 0 to 7
void set_txpower(bool high, uint8_t level) {
  aci_hal_set_tx_power_level(high, level);
}


void set_nondiscoverable() {
  aci_gap_set_non_discoverable();
  discoverable = false;
}



bool ble_loop() {
  HCI_Process();
  //ble_connection_state = connected;

  if (HCI_Queue_Empty()) {
    //
    //aci_hal_device_standby();
    return false;
  }


  return true;
}

uint8_t Write_UART_TX(const char* TXdata, uint8_t datasize)
{
  tBleStatus ret;

  ret = aci_gatt_update_char_value(UART_Serv_Handle, UART_RX_Char_Handle, 0, datasize, TXdata);

  //ret = aci_gatt_write_long_charac_val(UART_Serv_Handle, UART_RX_Char_Handle, 0, datasize, (const uint8_t *)TXdata);

  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error while updating UART characteristic.\n") ;
    return BLE_STATUS_ERROR ;
  }
  return BLE_STATUS_SUCCESS;

}

void Read_Request_CB(uint16_t handle)
{
  /*if(handle == UART_TX_Char_Handle + 1)
    {

    }
    else if(handle == UART_RX_Char_Handle + 1)
    {


    }*/

  if (connection_handle != 0)
    aci_gatt_allow_read(connection_handle);
}

void Attribute_Modified_CB(uint16_t handle, uint8_t data_length, uint8_t *att_data)
{
  if (handle == UART_TX_Char_Handle + 1) {


    //strncmp(att_data, "found", data_length)

    SerialUSB.write(att_data, data_length);
    SerialUSB.println();
    return;


    int i;
    for (i = 0; i < data_length; i++) {
      ble_rx_buffer[i] = att_data[i];
    }
    ble_rx_buffer[i] = '\0';
    ble_rx_buffer_len = data_length;

    if (ble_rx_buffer_len) {  //Check if data is available



      PRINTF("%d:%s\n", ble_rx_buffer_len, (char*)ble_rx_buffer);
      ble_rx_buffer_len = 0;//clear afer reading


      //const char *test_msg = "arduino sucks";
      //Write_UART_TX(test_msg, strlen(test_msg) + 1);
    }


  }
}

void GAP_ConnectionComplete_CB(uint8_t addr[6], uint16_t handle) {

  connected = TRUE;
  connection_handle = handle;

  PRINTF("Connected to device:");
  for (int i = 5; i > 0; i--) {
    PRINTF("%02X-", addr[i]);
  }
  PRINTF("%02X\r\n", addr[0]);
}

void GAP_DisconnectionComplete_CB(void) {
  connected = FALSE;
  PRINTF("Disconnected\n");
  /* Make the device connectable again. */
  set_discoverable();
}


// see STBlueNRG/hci.h
// will be called by HCI_Process
void HCI_Event_CB(void *pckt)
{
  hci_uart_pckt *hci_pckt = (hci_uart_pckt *)pckt;
  hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

  if (hci_pckt->type != HCI_EVENT_PKT)
    return;

  switch (event_pckt->evt) {

    case EVT_DISCONN_COMPLETE:
      {
        //auto *evt = (evt_disconn_complete *)event_pckt->data;
        GAP_DisconnectionComplete_CB();
      }
      break;

    case EVT_LE_META_EVENT:
      {
        evt_le_meta_event *evt = (evt_le_meta_event *)event_pckt->data;

        switch (evt->subevent) {
          case EVT_LE_CONN_COMPLETE:
            {
              evt_le_connection_complete *cc = (evt_le_connection_complete *)evt->data;
              GAP_ConnectionComplete_CB(cc->peer_bdaddr, cc->handle);
            }
            break;
        }
      }
      break;

    case EVT_VENDOR:
      {
        evt_blue_aci *blue_evt = (evt_blue_aci *)event_pckt->data;
        switch (blue_evt->ecode) {

          case EVT_BLUE_GATT_READ_PERMIT_REQ:
            {
              evt_gatt_read_permit_req *pr = (evt_gatt_read_permit_req *)blue_evt->data;
              Read_Request_CB(pr->attr_handle);
            }
            break;

          case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
            {
              evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)blue_evt->data;
              Attribute_Modified_CB(evt->attr_handle, evt->data_length, evt->att_data);
            }
            break;
        }
      }
      break;
  }

}
