#include "mgos.h"
#include "spi_flash.h"
#include "esp_rboot.h"

#define BLOCK_SIZE 4096
#define CHUNK_SIZE 4096
#define TEMP_STORAGE 0
#define DOWNLOAD_URL "https://raw.githubusercontent.com/yaourdt/mgos-to-tasmota/master/binary/tasmota.bin"

// current bootloader configuration
rboot_config *rboot_cfg;

/*
	TODO
	* hash verify download!
	* if failed wait 60 sec and reboot
	* disable wdt and interrupts during critical write operations
*/

/*
	src	src address
	dest	destination addess
	length	block length / byte
*/
void block_copy(uint32 src, uint32 dest, uint32 length) {
	LOG(LL_DEBUG, ("block_copy start: cp %d bytes from 0x%x to 0x%x", length, src, dest));
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
		state->status = *(int *) ev_data;
		break;
	case MG_EV_HTTP_CHUNK:
		// esp8266 expects flash to be erased and written in blocks of 4kb, but webservers send
		// chunks of data in whatever size they please. we thus buffer the data and write it
		// whenever a block is full
		state->next_blk      = ( state->dest + state->recieved + (uint32) hm->body.len ) / BLOCK_SIZE;
		state->left_in_block = ( (state->curr_blk + 1) * BLOCK_SIZE ) - state->dest - state->recieved;

		if ( state->next_blk > state->curr_blk ) {
			memcpy(&state->data[BLOCK_SIZE - state->left_in_block], hm->body.p, state->left_in_block);

			if ( spi_flash_erase_sector(state->curr_blk) != 0 ) {
				LOG(LL_ERROR, ("flash delete error! abort at %d recieved bytes.", state->recieved));
				c->flags |= MG_F_CLOSE_IMMEDIATELY;
				state->status = 500;
				break;
			}
			if ( spi_flash_write( state->curr_blk * BLOCK_SIZE, (uint32 *) state->data, BLOCK_SIZE) != 0 ) {
				LOG(LL_ERROR, ("flash write error! abort at %d recieved bytes.", state->recieved));
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
		// set status once file transfer is done
		state->status = hm->resp_code;
		c->flags |= MG_F_CLOSE_IMMEDIATELY;
		break;
	case MG_EV_CLOSE:
		// executed upon close connection
		LOG(LL_INFO, ("HTTP status is %d, recieved %d bytes", state->status, state->recieved));
		if (state->status == 200) {
			// write last block
			if ( spi_flash_erase_sector(state->curr_blk) != 0 ) {
				LOG(LL_ERROR, ("flash delete error! abort at %d recieved bytes.", state->recieved));
				state->status = 500;
				break;
			}
			state->left_in_block = ( (state->curr_blk + 1) * BLOCK_SIZE ) - state->dest - state->recieved;
			if ( spi_flash_write( state->curr_blk * BLOCK_SIZE, (uint32 *) state->data, BLOCK_SIZE - state->left_in_block) != 0 ) {
				LOG(LL_ERROR, ("flash write error! abort at %d recieved bytes.", state->recieved));
				state->status = 500;
				break;
			}
			LOG(LL_DEBUG, ("last block dump done"));

			block_copy( (*rboot_cfg).roms[TEMP_STORAGE], 0, state->recieved ); //TODO move me
			mgos_system_restart_after(200);
		} else {
			LOG(LL_ERROR, ("HTTP state not 200, abort!"));
		}
		break;
	}
};

/*
	url	src url
	dest	destination addess
*/
void download_file_to_flash(char *url, uint32 dest) {
	struct state *state;
	if ((state = calloc(1, sizeof(*state))) == NULL) {
		LOG(LL_ERROR, ("out of memory"));
		return;
	}
	state->dest     = dest;
	state->recieved = 0;
	state->curr_blk = dest / BLOCK_SIZE;

	LOG(LL_DEBUG, ("fetching %s to 0x%x", url, dest));
	if (!mg_connect_http(mgos_get_mgr(), http_cb, state, url, NULL, NULL)) {
		free(state);
		LOG(LL_ERROR, ("malformed URL"));
		return;
	}

	// TODO wait on download to finish, then continue here. also, return status
	// free(state);
	return;
};

// if we are online, lets download and flash tasmota
static void online_cb(int ev, void *evd, void *arg) {
	if ( ev == MGOS_NET_EV_IP_ACQUIRED ) {
		LOG(LL_INFO, ("device is online, downloading tasmota"));
		download_file_to_flash(DOWNLOAD_URL, (*rboot_cfg).roms[TEMP_STORAGE] );
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
