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
//
//-------------------------------------------------------------------------------

#pragma once

#include "arduino.h"

#include <utility>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cstddef>

#include <list>

#include "src/stble/ASTBLE/STBLE.h"


struct HCI_Event_CB_Info {
	void *data; 
	void (*cb)(void *, void *);
};
inline std::list<struct HCI_Event_CB_Info> HCI_Event_CBs;

void HCI_Event_CB(void *pckt) {
	for (auto info : ::HCI_Event_CBs)
		info.cb(info.data, pckt);
}

namespace stble {

inline bool process() {
	// nothing to process
	if (HCI_Queue_Empty())
		return false;
	HCI_Process();
	return true;
}

inline void register_event(void (*cb)(void *, void *), void *data = nullptr) {
	::HCI_Event_CBs.push_back(
		(struct HCI_Event_CB_Info) { .data = data, .cb = cb }
	);
}

struct public_address {
	tBDAddr addr;

	static struct public_address *make(tBDAddr base) {
		return (struct public_address *)base;
	}
} __attribute__((packed));

enum local_name_type : uint8_t {
	SHORT = AD_TYPE_SHORTENED_LOCAL_NAME,
	FULL  = AD_TYPE_COMPLETE_LOCAL_NAME
} __attribute__((packed));

template <std::size_t buf_len = 1>
struct local_name_string {
	char data[buf_len];

	local_name_string() : data({[0] = 0}) {}

	local_name_string(const char *s, std::size_t len) {
		this->assign(s, len);
	}

	local_name_string(const char *s) { this->assign(s); }

	local_name_string(const std::string &s) { this->assign(s); }

	local_name_string &assign(const char *s, std::size_t len) {
		std::memcpy(this->data, s, std::min(len, buf_len));
		return *this;
	}

	// null-terminated
	local_name_string &assign(const char *s) {
		return this->assign(s, strlen(s) + 1);
	}

	local_name_string &assign(const std::string &s) {
		return this->assign(s.c_str(), s.length() + 1);
	}

	// NOTE local name passed to aci_gap_set_discoverable is NOT null-terminated
	constexpr std::size_t size() const { return strlen(this->data); }

	constexpr std::size_t capacity() const { return buf_len; }
} __attribute__((packed));

// see aci_gap_set_discoverable
template <std::size_t buf_len = 40 - 14>
struct local_name {
	using type_e = enum local_name_type;
	using value_s = struct local_name_string<buf_len>;

	type_e type;
	value_s val;

	constexpr std::size_t size() { 
		return sizeof(this->type) + this->val.size(); 
	}
} __attribute__((packed));

struct ble_info {
	struct public_address addr;
};

struct uart_info {
	struct {
		uint16_t serv;
		uint16_t tx, rx;
	} handle;
};

class ble {
public:
	enum status_e {
		STATUS_SUCCESS,
		STATUS_FAILURE
	};
	
protected:
	struct {
		uint16_t serv;
		uint16_t dev_name_char;
		uint16_t appear_char;
	} handle;

	status_e set_pub_address(const struct public_address &addr) {
		if (aci_hal_write_config_data(
			CONFIG_DATA_PUBADDR_OFFSET, 
			sizeof(addr), (const uint8_t *)&addr
		) != BLE_STATUS_SUCCESS)
			return status_e::STATUS_FAILURE;
		return status_e::STATUS_SUCCESS;
	}

public:
	status_e init(const struct ble_info &info) {
		status_e s = status_e::STATUS_SUCCESS;
		tBleStatus s_ble;

		// init HCI
		HCI_Init();
		// init SPI interface 
		BNRG_SPI_Init();
		// reset BlueNRG/BlueNRG-MS SPI interface 
		BlueNRG_RST();

		s = this->set_pub_address(info.addr);
		if (s != status_e::STATUS_SUCCESS) {
			goto _loc_finally;
		}

		// init gatt
		s_ble = aci_gatt_init();
		if (s_ble != BLE_STATUS_SUCCESS) {
			s = status_e::STATUS_FAILURE;
			goto _loc_finally;
		}

		// init gap
		s_ble = aci_gap_init_IDB05A1(
			GAP_PERIPHERAL_ROLE_IDB05A1, 
			0, 
			7, 
			&this->handle.serv, 
			&this->handle.dev_name_char, 
			&this->handle.appear_char
		);
		if (s_ble != BLE_STATUS_SUCCESS) {
			s = status_e::STATUS_FAILURE;
			goto _loc_finally;
		}

	_loc_finally:
		return s;
	}

	status_e set_dev_name(const char *name, std::size_t len) {
		if (aci_gatt_update_char_value(
			this->handle.serv, this->handle.dev_name_char, 
			0, 
			len, name
		) != BLE_STATUS_SUCCESS)
			return status_e::STATUS_FAILURE;
		return status_e::STATUS_SUCCESS;
	}

	status_e set_dev_name(const char *name) {
		return this->set_dev_name(name, strlen(name));
	}

	status_e set_dev_name(const std::string &name) {
		return this->set_dev_name(name.c_str(), name.length());
	}
	
	// NOTE level: 0 to 7 dBm
	status_e set_txpower(bool high, uint8_t level_dbm) {
		if (aci_hal_set_tx_power_level(high, level_dbm) 
			!= BLE_STATUS_SUCCESS) 
			return status_e::STATUS_FAILURE;
		return status_e::STATUS_SUCCESS;
	}

	template <std::size_t name_cap>
	status_e set_discoverable(
		const struct local_name<name_cap> &name,
		std::size_t intvl_min_ms = 50,
		std::size_t intvl_max_ms = 100
	) {
		tBleStatus s_ble;

		s_ble = hci_le_set_scan_resp_data(0, nullptr);
		if (s_ble != BLE_STATUS_SUCCESS)
			return status_e::STATUS_FAILURE;

		s_ble = aci_gap_set_discoverable(
			ADV_IND,
			(intvl_min_ms * 1000) / 625, (intvl_max_ms * 1000) / 625,
			PUBLIC_ADDR, NO_WHITE_LIST_USE,
			name.size(), (const char *)&name, 
			0, nullptr, 
			0, 0
		);
		if (s_ble != BLE_STATUS_SUCCESS)
			return status_e::STATUS_FAILURE;

		return status_e::STATUS_SUCCESS;
	}

	status_e unset_discoverable() {
		if (aci_gap_set_non_discoverable() != BLE_STATUS_SUCCESS)
			return status_e::STATUS_FAILURE;
		return status_e::STATUS_SUCCESS;
	}

	std::pair<status_e, struct uart_info> add_service_uart() {
		const constexpr uint8_t uuid_type = UUID_TYPE_128;
		const constexpr uint8_t 
			uuid_uart_service[] = {
				0x9E, 0xCA, 0xDC, 0x24, 
				0x0E, 0xE5, 0xA9, 0xE0, 
				0x93, 0xF3, 0xA3, 0xB5,
				0x01, 0x00, 0x40, 0x6E
			},
			uuid_tx_char[] = {
				0x9E, 0xCA, 0xDC, 0x24, 
				0x0E, 0xE5, 0xA9, 0xE0, 
				0x93, 0xF3, 0xA3, 0xB5, 
				0x02, 0x00, 0x40, 0x6E	
			},
			uuid_rx_char[] = {
				0x9E, 0xCA, 0xDC, 0x24, 
				0x0E, 0xE5, 0xA9, 0xE0, 
				0x93, 0xF3, 0xA3, 0xB5, 
				0x03, 0x00, 0x40, 0x6E
			};

		struct uart_info info;

		status_e s = status_e::STATUS_SUCCESS;
		tBleStatus s_ble;

		// add service handle
		s_ble = aci_gatt_add_serv(
			uuid_type, uuid_uart_service, 
			PRIMARY_SERVICE, 7, 
			&info.handle.serv
		);
		if (s_ble != BLE_STATUS_SUCCESS) {
			s = status_e::STATUS_FAILURE;
			goto _loc_finally;
		}

		// add tx/rx handle
		s_ble = aci_gatt_add_char(
			info.handle.serv, 
			uuid_type, uuid_tx_char, 
			20, 
			CHAR_PROP_WRITE_WITHOUT_RESP, 
			ATTR_PERMISSION_NONE, 
			GATT_NOTIFY_ATTRIBUTE_WRITE,
			16, 
			1, 
			&info.handle.tx
		);
		if (s_ble != BLE_STATUS_SUCCESS) {
			s = status_e::STATUS_FAILURE;
			goto _loc_finally;
		}

		aci_gatt_add_char(
			info.handle.serv, 
			uuid_type, uuid_rx_char, 
			20, 
			CHAR_PROP_NOTIFY, 
			ATTR_PERMISSION_NONE, 
			GATT_DONT_NOTIFY_EVENTS,
			16, 
			1, 
			&info.handle.rx
		);
		if (s_ble != BLE_STATUS_SUCCESS) {
			s = status_e::STATUS_FAILURE;
			goto _loc_finally;
		}

	_loc_finally:
		return std::make_pair(s, info);
	}
};

class uart {
protected:
	struct uart_info info;	

	struct {
		uint16_t conn = 0;
	} handle;

	static void hci_handler(void *data, void *pckt) {
		auto *this_ = (class uart *)data;

		auto *hci_pckt = (hci_uart_pckt *)pckt;
		if (hci_pckt->type != HCI_EVENT_PKT)
			return;

		auto *event_pckt = (hci_event_pckt*)hci_pckt->data;
		switch (event_pckt->evt) {
		case EVT_DISCONN_COMPLETE: {
			auto *evt = (evt_disconn_complete *)event_pckt->data;

			if (this_->callbacks.disconnect != nullptr)
				this_->callbacks.disconnect(*evt);
		} break;
		case EVT_LE_META_EVENT: {
			auto *evt = (evt_le_meta_event *)event_pckt->data;

			switch (evt->subevent) {
			case EVT_LE_CONN_COMPLETE: {
				auto *cc = (evt_le_connection_complete *)evt->data;

				// TODO cc->peer_bdaddr_type;
				this_->handle.conn = cc->handle;
				if (this_->callbacks.connect != nullptr)
					this_->callbacks.connect(*cc);
			} break;
			}
		} break;
		case EVT_VENDOR: {
			auto *blue_evt = (evt_blue_aci *)event_pckt->data;

			switch (blue_evt->ecode) {
			case EVT_BLUE_GATT_READ_PERMIT_REQ: {
				auto *pr = (evt_gatt_read_permit_req *)blue_evt->data;
				if (this_->handle.conn == 0)
					break;
				// TODO multiple connections??
				if (this_->handle.conn != pr->conn_handle)
					break;

				// TODO err handling
				aci_gatt_allow_read(pr->conn_handle);
			} break;
			case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED: {
				auto *evt = (evt_gatt_attr_modified_IDB05A1 *)blue_evt->data;
				if (this_->handle.conn == 0)
					break;
				if (this_->handle.conn != evt->conn_handle)
					break;

				// ready for read
				if (evt->attr_handle == this_->info.handle.tx + 1) {
					if (this_->callbacks.read != nullptr)
						this_->callbacks.read((const char *)evt->att_data, evt->data_length);
				}

				// ready for write
				if (evt->attr_handle == this_->info.handle.rx + 1) {
					// TODO implement this
				}
			} break;
			}
		} break;
		}
	}

public:
	struct {
		void (*connect)(const evt_le_connection_complete &) = nullptr;
		void (*disconnect)(const evt_disconn_complete &) = nullptr;

		void (*read)(const char *, std::size_t) = nullptr;
		//void (*write)() = nullptr;
	} callbacks;

	enum status_e {
		STATUS_SUCCESS,
		STATUS_FAILURE
	};
	
	status_e init(const struct uart_info &info) {
		this->info = info;

		// register event handler
		register_event(this->hci_handler, this);

		return status_e::STATUS_SUCCESS;
	}

	status_e write(const char *data, std::size_t len) {
		tBleStatus s_ble;

		// TODO check if connected
		s_ble = aci_gatt_update_char_value(
			this->info.handle.serv, 
			this->info.handle.rx, 
			0, 
			len, data
		);
		if (s_ble != BLE_STATUS_SUCCESS) 
			return status_e::STATUS_FAILURE;

		return status_e::STATUS_SUCCESS;
	}

	status_e print(const char *s) {
		return this->write(s, strlen(s) + 1);
	}

	status_e print(const std::string &s) {
		return this->write(s.c_str(), s.length() + 1);
	}
};
}

namespace tinyzero {
inline stble::ble bluetooth;
};
