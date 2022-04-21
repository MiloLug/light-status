# light-status
## Compilation Requirements
```
make clang freetype2 libX11 libXft 
```

## Compilation
```sh
# run:
sudo make install
```

## Help
```sh
Usage: event-listener <event-source> <type> <code> <value> <command> [skip-count]
Example: event-listener /dev/input/event0 4 4 85 "shutdown -P now"

Parameters:
    <event-source> - string; event-source path
    <type>         - int; '*' = any
    <code>         - int; '*' = any
    <value>        - int; '*' = any
    <command>      - string; will be executed on match
    [skip-count]   - int; 0 by default; how many times to skip the event
        example: when it occurs two times and you want it to happen only once, just set skip-count to 1

The command's env variables:
    E_TIME_SEC     - int; timestamp's seconds
    E_TIME_USEC    - int; timestamp's remaining microseconds
    E_TYPE         - int
    E_CODE         - int
    E_VALUE        - int
```

