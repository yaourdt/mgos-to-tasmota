#include "mgos.h"
#include "spi_flash.h"
#include "esp_rboot.h"

#define BLOCK_SIZE 4096
#define CHUNK_SIZE 4096
#define TEMP_STORAGE 0

// current bootloader configuration
rboot_config *rboot_cfg;

/*
	TODO
	* hash verify download!
	* disable wdt and interrupts during critical write operations
	* move to esp flash write lib:
	#include "esp_flash_writer.h"
	static struct esp_flash_write_ctx s_wctx;
	...
	case MG_EV_CONNECT:
		esp_init_flash_write_ctx(0x2000, (0x100000 - 0x2000));
	...
	case MG_EV_HTTP_CHUNK:
		esp_flash_write(&s_wctx, hm->body);
*/

/*
	src	src address
	dest	destination addess
	length	block length / byte
*/
void block_copy(uint32 src, uint32 dest, uint32 length) {
	LOG(LL_DEBUG, ("block_copy start: cp %lu bytes from 0x%lx to 0x%lx", length, src, dest));
	uint32 chunk = CHUNK_SIZE, offset = 0;
	bool done    = false;
	char *data   = NULL;

	if (CHUNK_SIZE > BLOCK_SIZE || BLOCK_SIZE % CHUNK_SIZE != 0) {
		LOG(LL_ERROR, ("invalid CHUNK_SIZE. must be a divider of BLOCK_SIZE and < BLOCK_SIZE"));
		goto clean;
	}
	if ( src % BLOCK_SIZE != 0 || dest % BLOCK_SIZE != 0 ) {
		LOG(LL_ERROR, ("cannot copy: src or dest not aligned with fs block"));
		goto clean;
	}
	data = (char*) malloc(chunk);
	if (data == NULL) {
		LOG(LL_ERROR, ("out of memory"));
		goto clean;
	}

	while ( !done ) {
		if ( (long) (length - offset - chunk) <= 0 ) {
			chunk = length - offset;
			done  = true;
		}
		// erase new sector
		if ( offset % BLOCK_SIZE == 0 ) {
			if ( spi_flash_erase_sector( (dest + offset) / BLOCK_SIZE ) != 0 ) {
				LOG(LL_ERROR, ("flash delete error! abort."));
				goto clean;
			}
		}

		if ( spi_flash_read ( src  + offset, (uint32 *) data, chunk) != 0 ) {
			LOG(LL_ERROR, ("flash read error! abort."));
			goto clean;
		}
		if ( spi_flash_write( dest + offset, (uint32 *) data, chunk) != 0 ) {
			LOG(LL_ERROR, ("flash write error! abort."));
			goto clean;
		}
		offset = offset + chunk;
	}
	LOG(LL_DEBUG, ("block_copy done"));

	clean:
	if (data != NULL) { free(data); }
};

struct state {
	int status;                     // request status
	uint curr_blk, next_blk;	// current / next block to be written
	uint left_in_block;		// bytes left until current buffer is full
	uint32 recieved;                // number of bytes recieved
	uint32 dest;			// target flash address
	char data[BLOCK_SIZE];		// buffer for a block
};

static void http_cb(struct mg_connection *c, int ev, void *ev_data, void *ud) {
	struct http_message *hm = (struct http_message *) ev_data;
	struct state *state     = (struct state *) ud;

	switch (ev) {
	case MG_EV_CONNECT:
		// sent when a new outbound connection is created
		state->status = 0;
		break;
	case MG_EV_HTTP_CHUNK:
		// esp8266 expects flash to be erased and written in blocks of 4kb, but webservers send
		// chunks of data in whatever size they please. we thus buffer the data and write it
		// whenever a block is full
		if (hm->resp_code != 200) { break; }

		state->next_blk      = ( state->dest + state->recieved + (uint32) hm->body.len ) / BLOCK_SIZE;
		state->left_in_block = ( (state->curr_blk + 1) * BLOCK_SIZE ) - state->dest - state->recieved;

		if ( state->next_blk > state->curr_blk ) {
			memcpy(&state->data[BLOCK_SIZE - state->left_in_block], hm->body.p, state->left_in_block);

			// check if the next write will overwrite the currently running program
			if ( (state->next_blk * BLOCK_SIZE) > (*rboot_cfg).roms[(*rboot_cfg).current_rom] ) {
				LOG(LL_ERROR, ("error! operation would overwrite the currently running program."));
				c->flags |= MG_F_CLOSE_IMMEDIATELY;
				state->status = 500;
				break;
			}

			if ( spi_flash_erase_sector(state->curr_blk) != 0 ) {
				LOG(LL_ERROR, ("flash delete error! abort at %lu recieved bytes.", state->recieved));
				c->flags |= MG_F_CLOSE_IMMEDIATELY;
				state->status = 500;
				break;
			}
			if ( spi_flash_write( state->curr_blk * BLOCK_SIZE, (uint32 *) state->data, BLOCK_SIZE) != 0 ) {
				LOG(LL_ERROR, ("flash write error! abort at %lu recieved bytes.", state->recieved));
				c->flags |= MG_F_CLOSE_IMMEDIATELY;
				state->status = 500;
				break;
			}
			state->curr_blk = state->next_blk;

			memcpy(&state->data[0], hm->body.p + state->left_in_block, hm->body.len - state->left_in_block);
		} else {
			memcpy(&state->data[BLOCK_SIZE - state->left_in_block], hm->body.p, hm->body.len);
		}
		state->recieved += (uint32) hm->body.len;
		c->flags |= MG_F_DELETE_CHUNK;
		break;
	case MG_EV_HTTP_REPLY:
		if (hm->resp_code == 302) {
			// follow http redirect ...
			for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
				if ( mg_strcasecmp(hm->header_names[i], mg_mk_str("location") ) == 0 ) {
					LOG(LL_DEBUG, ("302 redirect to %.*s", hm->header_values[i].len, hm->header_values[i].p));

					char *url;
					asprintf(&url, "%.*s", hm->header_values[i].len, hm->header_values[i].p);
					mg_connect_http(mgos_get_mgr(), http_cb, state, url, NULL, NULL);
					break;
				}
			}
		} else {
			// ...or set status once file transfer is done
			state->status = hm->resp_code;
		}

		c->flags |= MG_F_CLOSE_IMMEDIATELY;
		break;
	case MG_EV_CLOSE:
		// executed upon close connection
		LOG(LL_INFO, ("HTTP status is %d, recieved %lu bytes", state->status, state->recieved));
		if (state->status == 200) {
			// write last block
			if ( spi_flash_erase_sector(state->curr_blk) != 0 ) {
				LOG(LL_ERROR, ("flash delete error! abort at %lu recieved bytes.", state->recieved));
				break;
			}
			state->left_in_block = ( (state->curr_blk + 1) * BLOCK_SIZE ) - state->dest - state->recieved;
			if ( spi_flash_write( state->curr_blk * BLOCK_SIZE, (uint32 *) state->data, BLOCK_SIZE - state->left_in_block) != 0 ) {
				LOG(LL_ERROR, ("flash write error! abort at %lu recieved bytes.", state->recieved));
				break;
			}
			LOG(LL_DEBUG, ("last block dump done"));

			// check if the next operation will overwrite the currently running program
			if ( ((state->curr_blk + 3) * BLOCK_SIZE) > (*rboot_cfg).roms[(*rboot_cfg).current_rom] ) {
				LOG(LL_ERROR, ("error! operation would overwrite the currently running program."));
				break;
			}

			// we are going to overwrite the bootloader and boot config last
			// in case something goes wrong. to do so, we first need to save
			// respective blocks, so they don't get overwritten by the main
			// copy process

			// save bootloader block
			block_copy(
				(*rboot_cfg).roms[TEMP_STORAGE],
				(state->curr_blk + 1) * BLOCK_SIZE,
				BLOCK_SIZE
			);

			// save boot config block
			block_copy(
				(*rboot_cfg).roms[TEMP_STORAGE] + BOOT_CONFIG_ADDR,
				(state->curr_blk + 2) * BLOCK_SIZE,
				BLOCK_SIZE
			);

			// move everything between the bootloader and boot config
			block_copy(
				(*rboot_cfg).roms[TEMP_STORAGE] + BLOCK_SIZE,
				BLOCK_SIZE,
				BOOT_CONFIG_ADDR - BLOCK_SIZE
			);

			// move everything after boot config
			block_copy(
				(*rboot_cfg).roms[TEMP_STORAGE] + BOOT_CONFIG_ADDR + BLOCK_SIZE,
				BOOT_CONFIG_ADDR + BLOCK_SIZE,
				state->recieved - BOOT_CONFIG_ADDR - BLOCK_SIZE
			);

			// overwrite boot config
			block_copy(
				(state->curr_blk + 2) * BLOCK_SIZE,
				BOOT_CONFIG_ADDR,
				BLOCK_SIZE
			);

			// overwrite boot loader
			block_copy(
				(state->curr_blk + 1) * BLOCK_SIZE,
				0,
				BLOCK_SIZE
			);

			mgos_system_restart_after(200);
		} else if (state->status == 0) {
			LOG(LL_INFO, ("Following HTTP redirect..."));
		} else {
			LOG(LL_ERROR, ("HTTP response error: %d", state->status));
		}
		break;
	}
};

/*
	url	src url
	dest	destination addess
*/
void download_file_to_flash(const char *url, uint32 dest) {
	struct state *state;
	if ((state = calloc(1, sizeof(*state))) == NULL) {
		LOG(LL_ERROR, ("out of memory"));
		return;
	}
	state->dest     = dest;
	state->recieved = 0;
	state->curr_blk = dest / BLOCK_SIZE;

	LOG(LL_DEBUG, ("fetching %s to 0x%lx", url, dest));
	mg_connect_http(mgos_get_mgr(), http_cb, state, url, NULL, NULL);
	return;
};

// if we are online, lets download and flash the target firmware
static void online_cb(int ev, void *evd, void *arg) {
	if ( ev == MGOS_NET_EV_IP_ACQUIRED ) {
		LOG(LL_INFO, ("device is online, downloading target firmware"));
		download_file_to_flash(mgos_sys_config_get_mg2x_url(), (*rboot_cfg).roms[TEMP_STORAGE] );
	}
	(void) evd;
	(void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
	// get current bootloader configuration
	rboot_cfg = get_rboot_config();

	// if we are running from app0, move flash content to app1 (and fs0 -> fs1) and reboot
	if ( (*rboot_cfg).current_rom == 0 ) {
		LOG(LL_INFO, ("FW booted from app0, copy to app1 and reboot"));
		block_copy( (*rboot_cfg).roms[0],         (*rboot_cfg).roms[1],         (*rboot_cfg).roms_sizes[0] );
		block_copy( (*rboot_cfg).fs_addresses[0], (*rboot_cfg).fs_addresses[1], (*rboot_cfg).fs_sizes[0] );

		// update bootloader configuration
		(*rboot_cfg).current_rom   = 1;
		(*rboot_cfg).previous_rom  = 0;
		(*rboot_cfg).fw_updated    = 1;
		(*rboot_cfg).is_first_boot = 1;

		rboot_set_config(rboot_cfg);

		mgos_system_restart_after(200);
	} else {
		LOG(LL_INFO, ("FW booted from app1, waiting for network connection"));
		mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, online_cb, NULL);
	}
	return MGOS_APP_INIT_SUCCESS;
}
