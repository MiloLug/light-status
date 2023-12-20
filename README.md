# light-status

A small utility that allows you to create basic floating windows to output any one-line text in them.

It can be used to create some system UI, based on scripts and outputs of other programs (slstatus etc.):
[![HKrurAP.md.png](https://iili.io/HKrurAP.md.png)](https://freeimage.host/i/HKrurAP)

## Compilation Requirements
```
make clang freetype2 libX11 libXft fontconfig libXinerama
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

        XORG PROPERTIES
    -Xn <name>          - window name
    -Xc <class>         - window class

<data-command> is a command that will be executed with popen() to show its output.
    The command should periodically return a value, for example:
        "while true; do echo `date`; sleep 1; done"
    or
        "slstatus -s"

Alignment <value> can be:
    C - center
    U - unset (default)
    <number> - offset in pixels

<color> should be in hex format with leading # (#000fff)

<font> should be in pattern: <font-name>[:size=<font-size>]
    <font-name> can be:
       actual name
       font family name - monospace, sans, etc.

```
