#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <stdint.h>

/**
 * Initialize serial command interface
 * Starts a task that listens for commands on stdin
 */
void serial_commands_init(void);

/**
 * Print help message with available commands
 */
void serial_commands_print_help(void);

/**
 * Check if VU meter is enabled
 */
bool is_vu_meter_enabled(void);

#endif // SERIAL_COMMANDS_H
