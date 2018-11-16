# The Monitor-Monitor

A physical indicator for which physical display hold the currently active application

## Notes

Fetch correct URB device:

```bash
ioreg -c IOSerialBSDClient -r -t \
  | awk 'f;/com_silabs_driver_CP210xVCPDriver/{f=1};/IOCalloutDevice/{exit}' \
  | sed -n 's/.*"\(\/dev\/.*\)".*/\1/p'
```

Example commands:

```plaintext
add one 0
add two 1
list
set one
set two
remove one
remove two
```
