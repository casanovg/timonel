__UploadApplication page-address calculation selection logic__

while (THERE_IS_PAYLOAD_DATA_NOT_UPLOADED) {

    if (AUTO_PAGE_ADDR != enabled) {

        // The bootloader doesn't calculate page addresses by itself - Special operation mode -
            
        if (CMD_SETPGADDR != enabled) {

            // The bootloader doesn't have the set page address command enabled,
            // so, it doesn't allow the TWI master to set addresses.

            return(RETURN_WITH_ERROR);

        }

        INCREMENT_PAGE_ADDRESS();

        if ((PAGE_ADDRESS == TRAMPOLINE_PAGE) && (APP_USE_TPL_PG == enabled)) {

            MODIFY_PAGE_DATA_TO_INSERT_TRAMPOLINE_BYTES();

        }

    } else {

        // The bootloader calculates page addresses by itself - Normal operation mode - 

        UPLOAD_PAGE_DATA();

    }

}

APP_USE_TPL_PG
