#include <boot/pongo.h>

// HUGE thanks to @mineekdev for their openra1n project,
// which was the template for this code.

bool preparePongoOS()
{
    FILE *shellcodeFile, *pongoFile;
    size_t shellcodeSize, pongoSize;
    void *shellcode, *pongo;

    // The shellcode that is appended to the beginning of the
    // LZ4-compressed Pongo is actually an LZ4 decompressor
    // that decompresses the Pongo image into memory.
    // It is, in effect, a self-extracting payload.
    // Get shellcode
    shellcodeFile = fopen("src/boot/payloads/checkra1n/shellcode.bin", "rb");
    if (shellcodeFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open shellcode file");
        return false;
    }
    fseek(shellcodeFile, 0, SEEK_END);
    shellcodeSize = ftell(shellcodeFile);
    rewind(shellcodeFile);

    shellcode = malloc(shellcodeSize);
    fread(shellcode, shellcodeSize, 1, shellcodeFile);
    fclose(shellcodeFile);


    // Get PongoOS
    pongoFile = fopen("src/boot/payloads/checkra1n/Pongo.bin", "rb");
    if (pongoFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
        return false;
    }
    fseek(pongoFile, 0, SEEK_END);
    pongoSize = ftell(pongoFile);
    rewind(pongoFile);
    pongo = malloc(pongoSize);
    fread(pongo, pongoSize, 1, pongoFile);
    fclose(pongoFile);

    // Compress PongoOS
    char *pongoCompressed = malloc(pongoSize);
    LOG(LOG_DEBUG, "Compressing PongoOS");
    pongoSize = LZ4_compress_HC(pongo, pongoCompressed, pongoSize, pongoSize, LZ4HC_CLEVEL_MAX);
    if (pongoSize == 0) {
        LOG(LOG_ERROR, "ERROR: failed to compress PongoOS");
        return false;
    }

    // Add shellcode to PongoOS
    LOG(LOG_DEBUG, "Adding shellcode to PongoOS");
    void *tmp = malloc(pongoSize + shellcodeSize);
    memcpy(tmp, shellcode, shellcodeSize);
    memcpy(tmp + shellcodeSize, pongoCompressed, pongoSize);
    free(pongo);
    pongo = tmp;
    pongoSize += shellcodeSize;
    free(shellcode);

    // Write size of compressed Pongo into data for decompressor
    uint32_t *pongoSizeInData = (uint32_t *)(pongo + 0x1fc);
    *pongoSizeInData = pongoSize - shellcodeSize;

    // Write PongoOS to file
    pongoFile = fopen("src/boot/payloads/checkra1n/PongoShellcode.bin", "wb+");
    if (pongoFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
        return false;
    }
    fwrite(pongo, pongoSize, 1, pongoFile);
    fclose(pongoFile);

    return true;
}

bool bootPongoOS(device_t *device)
{
    FILE *pongoFile;
    void *pongoOS;
    size_t pongoSize;
    transfer_ret_t ret;
    preparePongoOS();
    if (pongoOS == NULL) {
        LOG(LOG_ERROR, "ERROR: failed to get PongoOS");
        return false;
    }

    // TODO: a better way to do this without file IO
    // Get PongoOS
    pongoFile = fopen("src/boot/payloads/checkra1n/PongoShellcode.bin", "rb");
    if (pongoFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
        return false;
    }
    fseek(pongoFile, 0, SEEK_END);
    pongoSize = ftell(pongoFile);
    rewind(pongoFile);
    pongoOS = malloc(pongoSize);
    fread(pongoOS, pongoSize, 1, pongoFile);
    fclose(pongoFile);
    remove("src/boot/payloads/checkra1n/PongoShellcode.bin");

    char *serial = getDeviceSerialNumberWithTransfer(&device->handle);
    if (serial == NULL) {
        LOG(LOG_ERROR, "ERROR: failed to get device serial number");
        return false;
    }

    LOG(LOG_DEBUG, "Sending PongoOS of size 0x%X", pongoSize);
    {
        size_t lengthSent = 0, size;
        while (lengthSent < pongoSize) 
        {
            retry:
                size = ((pongoSize - lengthSent) > 0x800) ? 0x800 : (pongoSize - lengthSent);
                sendUSBControlRequest(&device->handle, 0x21, DFU_DNLOAD, 0, 0, (unsigned char *)&pongoOS[lengthSent], size, &ret);
                if (ret.sz != size || ret.ret != USB_TRANSFER_OK) {
                    LOG(LOG_DEBUG, "Retrying at length 0x%X", lengthSent);
                    sleep_ms(100);
                    goto retry;
                }
                lengthSent += size;
        }
    }
    sendUSBControlRequestNoData(&device->handle, 0x21, DFU_CLRSTATUS, 0, 0, 0, NULL);
    return true;
}

bool pongoOSHasBooted(char *serial) {
    if (serial == NULL) {
        LOG(LOG_ERROR, "ERROR: failed to get device serial number");
        return false;
    }
    if (strstr(serial, "SRTG:[PongoOS") != NULL) {
        return true;
    }
    return false;
}