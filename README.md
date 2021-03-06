# light-status
## Compilation Requirements
```
make clang freetype2 libX11 libXft fontconfig 
```

## Compilation
```sh
# run:
sudo make install
```

## Help
```sh
Usage: light-status [flags]

Flags:
    --help              - display help
    -i <data-command>   - data collection command

        PANEL CONFIG
    -w <width>          - panel width
    -h <height>         - panel height
    -[l,r,t,b] <value>  - panel left, right, top and bottom alignment
    -c <color>          - panel color

        TEXT CONFIG
    -T[l,r,t,b] <value> - text left, right, top and bottom alignment
    -Tf <font>          - font pattern
    -Tc <color>         - text color

<data-command> is a command that will be executed with popen() to show its output.
    The command should periodically return a value, for example:
        "while true; do echo `date`; sleep 1; done"
    or
        "slstatus -s"

Alignment <value> can be:
    C - center
    U - unset
    <number> - offset in pixels

<color> should be in hex format with leading # (#000fff)

<font> should be in pattern: <font-name>[:size=<font-size>]
    <font-name> can be:
       actual name
       font family name - monospace, sans, etc.

```

