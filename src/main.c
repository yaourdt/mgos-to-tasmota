#include "mgos.h"
#include "spi_flash.h"
#include "esp_rboot.h"

#define BLOCK_SIZE 4096
#define CHUNK_SIZE 4096

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
			spi_flash_erase_sector( (dest + offset) / BLOCK_SIZE );
		}

		spi_flash_read ( src  + offset, (uint32 *) data, chunk);
		spi_flash_write( dest + offset, (uint32 *) data, chunk);
		offset = offset + chunk;
	}
	LOG(LL_DEBUG, ("block_copy done"));

	clean:
		if (data != NULL) { free(data); }
}

enum mgos_app_init_result mgos_app_init(void) {
	// get current bootloader configuration
	rboot_config *rboot_cfg;
	rboot_cfg = get_rboot_config();

	// if we are running from app0, move flash content to app1 (and fs0 -> fs1) and reboot
	LOG(LL_INFO, ("******** current_rom     %d", (*rboot_cfg).current_rom ));
	LOG(LL_INFO, ("******** previous_rom    %d", (*rboot_cfg).previous_rom ));
	LOG(LL_INFO, ("******** fw_updated      %d", (*rboot_cfg).fw_updated ));
	LOG(LL_INFO, ("******** is_first_boot   %d", (*rboot_cfg).is_first_boot ));
	LOG(LL_INFO, ("******** roms[0]         %d", (*rboot_cfg).roms[0] ));
	LOG(LL_INFO, ("******** roms[1]         %d", (*rboot_cfg).roms[1] ));
	LOG(LL_INFO, ("******** roms_sizes[0]   %d", (*rboot_cfg).roms_sizes[0] ));
	//LOG(LL_INFO, ("******** fs_addresses[0] %d", (*rboot_cfg).fs_addresses[0] ));
	//LOG(LL_INFO, ("******** fs_addresses[1] %d", (*rboot_cfg).fs_addresses[1] ));
	//LOG(LL_INFO, ("******** fs_sizes[0]     %d", (*rboot_cfg).fs_sizes[0] ));

	if ( (*rboot_cfg).current_rom == 0 ) {
		LOG(LL_INFO, ("FW booted from app0, copy to app1 and reboot"));
		block_copy( (*rboot_cfg).roms[0],         (*rboot_cfg).roms[1],         (*rboot_cfg).roms_sizes[0] );
		block_copy( (*rboot_cfg).fs_addresses[0], (*rboot_cfg).fs_addresses[1], (*rboot_cfg).fs_sizes[0] );

		// update bootloader configuration
		(*rboot_cfg).current_rom   = 1;
		(*rboot_cfg).previous_rom  = 0;
		(*rboot_cfg).fw_updated    = 1;
		(*rboot_cfg).is_first_boot = 1;

		bool success = rboot_set_config(rboot_cfg);
		LOG(LL_INFO, ("*******x success       %d", success ));

		mgos_system_restart_after(20000);
	} else if ( (*rboot_cfg).current_rom == 1 ) {
		LOG(LL_INFO, ("FW booted from app1"));
	}
	return MGOS_APP_INIT_SUCCESS;
}
