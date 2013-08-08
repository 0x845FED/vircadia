//
//  inv_tty.c
//  interface
//
//  Created by Andrzej Kapolka on 7/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "inv_tty.h"

// the file descriptor of the tty
static int ttyFileDescriptor;

void tty_set_file_descriptor(int file_descriptor) {
    ttyFileDescriptor = file_descriptor;
}

static char to_hex_digit(unsigned char value) {
    return (value < 10) ? '0' + value : 'A' + (value - 10);
}

static unsigned char from_hex_digit(char digit) {
    return (digit < 'A') ? digit - '0' : (digit - 'A') + 10;
}

static int write_byte(unsigned char value) {
    char chars[] = { to_hex_digit(value / 16), to_hex_digit(value % 16) };
    return write(ttyFileDescriptor, chars, 2) != 2;
}

static int read_byte(unsigned char* value) {
    char chars[2];
    if (read(ttyFileDescriptor, chars, 2) != 2) {
        return 1;
    }
    *value = from_hex_digit(chars[0]) * 16 + from_hex_digit(chars[1]);
    return 0;
}

int tty_i2c_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char const *data) {
    if (write(ttyFileDescriptor, "WR", 2) != 2) {
        return 1;
    }
    if (write_byte(slave_addr)) {
        return 1;
    }
    if (write_byte(reg_addr)) {
        return 1;
    }
    int i;
    for (i = 0; i < length; i++) {
        if (write_byte(data[i])) {
            return 1;
        }
    }
    if (write(ttyFileDescriptor, "\n", 1) != 1) {
        return 1;
    }
    
    char response[8];
    return read(ttyFileDescriptor, response, 8) != 8;
}

int tty_i2c_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data) {
    if (write(ttyFileDescriptor, "RD", 2) != 2) {
        return 1;
    }
    if (write_byte(slave_addr)) {
        return 1;
    }
    if (write_byte(reg_addr)) {
        return 1;
    }
    if (write_byte(length)) {
        return 1;
    }
    if (write(ttyFileDescriptor, "\n", 1) != 1) {
        return 1;
    }
    
    char prefix[6];
    if (read(ttyFileDescriptor, prefix, 6) != 6) {
        return 1;
    }
    int i;
    for (i = 0; i < length; i++) {
        if (read_byte(data + i)) {
            return 1;
        }
    }
    
    char suffix[2];
    return read(ttyFileDescriptor, suffix, 2) != 2;
}

void tty_delay_ms(unsigned long num_ms) {
    struct timespec required, remaining;
    required.tv_sec = 0;
    const long NANOSECONDS_PER_MILLISECOND = 1000000;
    required.tv_nsec = num_ms * NANOSECONDS_PER_MILLISECOND;
    nanosleep(&required, &remaining);
}

void tty_get_ms(unsigned long *count) {
    struct timeval time;
    gettimeofday(&time, 0);
    const long MILLISECONDS_PER_SECOND = 1000;
    const long MICROSECONDS_PER_MILLISECOND = 1000;
    *count = time.tv_sec * MILLISECONDS_PER_SECOND + time.tv_usec / MICROSECONDS_PER_MILLISECOND; 
}
