#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "sd.h"

// Applies a simple XOR encryption/deceyption key to the device data buffer
void apply_xor_encryption(struct my_device_data *data, int key) {
    int i;
    for (i = 0; i < data->size; i++) {
        data->buffer[i] ^= key;
    }
}

// SHA-1 Bit move macro
#define SHA1_ROTL(bits, word) (((word) << (bits)) | ((word) >> (32 - (bits))))

int apply_sha1_hash(struct my_device_data *data) {
    // Initialize SHA-1 constants
    u32 h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    
    u64 ml = (u64)data->size * 8; // Text length in bits
    size_t padded_len = data->size + 1;
    u8 *msg;
    size_t offset;
    int i;
    
    // Text padding amount
    while (padded_len % 64 != 56) {
        padded_len++;
    }
    padded_len += 8; // add 8 bytes for the 64-bit message length trailer
    
    // Allocate temporary kernel memory for the padded message buffer
    msg = kmalloc(padded_len, GFP_KERNEL);
    if (!msg) return -ENOMEM;
    
    memset(msg, 0, padded_len);
    memcpy(msg, data->buffer, data->size);
    
    // Append mandatory single '1' bit
    msg[data->size] = 0x80;
    
    // Append the original message length in bits as a 64-bit-endian int
    for (i = 7; i >= 0; --i) {
        msg[padded_len - 1 - i] = (ml >> (i * 8)) & 0xFF;
    }
    
    // Process padded msg in 512-bit (64-byte) blocks
    for (offset = 0; offset < padded_len; offset += 64) {
        u32 w[80];
        u32 a = h0, b = h1, c = h2, d = h3, e = h4;
        
        // Break into sixteen 32-bit words
        for (i = 0; i < 16; ++i) {
            w[i] = (msg[offset + i*4] << 24) | (msg[offset + i*4 + 1] << 16) | 
                   (msg[offset + i*4 + 2] << 8) | (msg[offset + i*4 + 3]);
        }
        // Expand sixteen 32-bit words into eighty 32-bit words
        for (i = 16; i < 80; ++i) {
            w[i] = SHA1_ROTL(1, w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]);
        }
        
        // Main SHA-1 mixing loop
        for (i = 0; i < 80; ++i) {
            u32 f, k, temp;
            if (i < 20) { 
                f = (b & c) | ((~b) & d); 
                k = 0x5A827999; 
            } else if (i < 40) { 
                f = b ^ c ^ d; 
                k = 0x6ED9EBA1; 
            } else if (i < 60) { 
                f = (b & c) | (b & d) | (c & d); 
                k = 0x8F1BBCDC; 
            } else { 
                f = b ^ c ^ d; 
                k = 0xCA62C1D6; 
            }

            temp = SHA1_ROTL(5, a) + f + e + k + w[i];
            e = d; 
            d = c; 
            c = SHA1_ROTL(30, b); 
            b = a; 
            a = temp;
        }
        // Accumulate intermediate block hash results into global state words
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    
    // release temporary kernel message padding buffer
    kfree(msg);
    
    // Shift to bytes and to buffer
    memset(data->buffer, 0, BUF_SIZE);
    u32 h[5] = {h0, h1, h2, h3, h4};
    
    for (i = 0; i < 5; ++i) {
        data->buffer[i*4]     = (h[i] >> 24) & 0xFF;
        data->buffer[i*4 + 1] = (h[i] >> 16) & 0xFF;
        data->buffer[i*4 + 2] = (h[i] >> 8)  & 0xFF;
        data->buffer[i*4 + 3] = h[i]         & 0xFF;
    }
    
    data->size = SHA1_LENGTH;
    
    return 0;
}


// Manual base encode
int apply_base64_encode(struct my_device_data *data) {
    const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0, out_idx = 0;
    unsigned char char_array_3[3], char_array_4[4];
    unsigned char *bytes = (unsigned char *)data->buffer;
    size_t len = data->size;
    char *temp_out;
    size_t encoded_len;
    
    if (len == 0) return 0;

    // Base64 grows size: (len + 2) / 3 * 4
    encoded_len = 4 * ((len + 2) / 3);
    
    // Ensure no growth(overflow) above buffer size
    if (encoded_len >= BUF_SIZE) {
        return -ENOSPC; 
    }

    // Get temporary memory in kernel space
    temp_out = kmalloc(encoded_len + 1, GFP_KERNEL);
    if (!temp_out) return -ENOMEM;

    // Convert input bytes 3 ata time into 4 base64 chars
    while (len--) {
        char_array_3[i++] = *(bytes++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) {
                temp_out[out_idx++] = b64chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    // Handle any remaining trailing bytes
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) {
            temp_out[out_idx++] = b64chars[char_array_4[j]];
        }
        while (i++ < 3) {
            temp_out[out_idx++] = '=';
        }
    }

    // Transfer ready Base64-text back to device buffer
    memset(data->buffer, 0, BUF_SIZE);
    memcpy(data->buffer, temp_out, encoded_len);
    data->size = encoded_len;

    // release temporary memory
    kfree(temp_out);
    
    return 0;
}