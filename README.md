# The Monitor-Monitor

A physical indicator for which physical display hold the currently active application

## Commands

The monitor-monitor features a simple serial command system to configure and set the current mode.
Commands may take a number of parameters. There is no serial echo, this is meant to be executed by
another program, however it is usable by careful typing. The delete and backspace characters are not
recognized.

Example:

```plaintext
add one 0
add two 1
list
set one
set two
remove one
remove two
```

On macOS, you can fetch the correct USB device with the following command.

```bash
ioreg -c IOSerialBSDClient -r -t \
  | awk 'f;/com_silabs_driver_CP210xVCPDriver/{f=1};/IOCalloutDevice/{exit}' \
  | sed -n 's/.*"\(\/dev\/.*\)".*/\1/p'
```

### `help`

Print a help screen with a summary of the commands

### `list`

Show all screens as configured

### `show`

Read the config file and pretty-print to the terminal

### `add <name> <index>`

Adds a new physical indicator to the set. The name is intended to be a unique identifier which can
be determined by the host machine at runtime, usually the hardware identifier on macOS. The index is
which physical indicator it is along the daisy-chained set of indicators.

This command will save back to the hardware's built-in filesystem.

### `remove <name>`

Removes an indicator from the set.

This command will save back to the hardware's built-in filesystem.

### `set <name>`

Look up the given name in the loaded set and set it as the currently active indicator. If another
indicator is active, fade that one out first. If the given name doesn't exist, unset the current
indicator so all are off.

### `color <hue> <saturation> <intensity>`

Set the indicator color to the given HSI value. Hue is given as a floating point angle between 0 and
360, saturation is a floating point percentage between 0 and 1, and intensity is an integer between
0 and 256.

This command will save back to the hardware's built-in filesystem.

### `reset`

Reset the indicator color to the default.

This command will save back to the hardware's built-in filesystem.
