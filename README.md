# editwl

> [C++ libraries and PC tools for Nintendo DS(i) ROM formats/editing](https://xortroll.github.io/editwl/)

- [editwl](#editwl)
  - [libeditwl](#libeditwl)
    - [Supported formats](#supported-formats)
  - [editwl-bin](#editwl-bin)
    - [UI](#ui)
    - [CLI](#cli)
  - [Building](#building)
    - [libnedit](#libnedit)
    - [editwl-bin](#editwl-bin-1)
  - [TODO](#todo)
    - [libeditwl](#libeditwl-1)
  - [Support](#support)
  - [Credits](#credits)

## libeditwl

[`libeditwl`](libeditwl) libraries are the core component of this project. They consist on lightweight C++ libraries for DS(i) ROM format reading, writing, editing and more.

While the libraries are not properly documented yet, it can be helpful to check [editwl-bin modules](editwl-bin/modules) as examples.

Note that these libraries work by loading everything (entire ROMs/files/etc) into memory (similar to other existing DS(i) tools/libraries). In worst cases (loading big ROMs with compressed files), the code may allocate hundreds of MB to store everything.

### Supported formats

- BMG

- DS(i) ROMs

- NARC archives (or CARC/ARC)

- DWC utility archives ("utility.bin")

## editwl-bin

[`editwl-bin`](editwl-bin) is (yet another) desktop Nintendo DS(i) ROM editor, made from scratch and inspired on already existing ones, aiming to mimic the best of them. It is modular, hence it may be extended with custom format modules.

This tool may be used as both a graphical editor (by opening it with no arguments or a single file argument, typically by drag-dropping the file in the executable from a UI file browser) or as a command-line tool, getting the best of both worlds.

### UI

Features:

- BMG

  - Edit messages and other fields (file ID and so on), with a custom format to handle escaping

  - Save/load in a custom XML format for easier message editing (see [docs](https://xortroll.github.io/editwl/)).

### CLI

- BMG: `editwl-bin bmg`

  - List messages: `editwl-bin bmg list -i/--in=<bmg-file> [-v/--verbose]`

    - Example: `editwl-bin bmg list --in=data.bmg`
  
  - Get (print) specific message: `editwl-bin bmg get -i/--in=<bmg-file> --idx=<msg-idx>`

    - Example (get second message, thus index `1`): `editwl-bin get -i file.bmg --idx=1`

  - Create BMG: `editwl-bin bmg create -i/--in=<bmg-xml-file> -o/--out=<bmg-file>`

    - Example: `editwl-bin bmg create --in=msgs.xml -o gen.bmg`

  - Convert BMG (to XML): `editwl-bin bmg convert -i/--in=<bmg-file> -o/--out=<xml-file>`

    - Example: `editwl-bin bmg convert -i data.bmg -o plain_data.xml`

- ROM: `editwl-bin rom`

  - Show ROM information: `editwl-bin rom info -r/--rom=<rom-file>`

    - Example: `editwl-bin rom info -r game.nds`

  - Extract (binary) header: `editwl-bin rom extract-header -r/--rom=<rom-file> -o/--out=<header-bin-file>`

    - Example: `editwl-bin rom extract-header --rom=rom.nds --out=header.bin`

  - Extract (binary) overlay table: `editwl-bin rom extract-overlay-table -r/--rom=<rom-file> -p/--proc=<processor> -o/--out=<ovt-bin-file>`

    - Example: `editwl-bin rom extract-overlay-table --rom=rom.nds --proc=arm9 --out=ovt.bin`

  - Extract (binary) overlay tables: `editwl-bin rom extract-overlay-tables -r/--rom=<rom-file> -7/--out7=<ovt7-bin-file> -9/--out9=<ovt9-bin-file>`

    - Example: `editwl-bin rom extract-overlay-tables --rom=rom.nds --out7=ovt7.bin -9 ovt9.bin`

  - Extract (binary) code: `editwl-bin rom extract-code -r/--rom=<rom-file> -p/--proc=<processor> -o/--out=<code-bin-file>`

    - Example: `editwl-bin rom extract-code --rom=rom.nds --proc=arm7 --out=arm7.bin`

  - Extract (binary) codes: `editwl-bin rom extract-codes -r/--rom=<rom-file> -7/--out7=<code7-bin-file> -9/--out9=<code9-bin-file>`

    - Example: `editwl-bin rom extract-codes --rom=rom.nds --out7=arm7.bin -9 arm9.bin`

  - Replace (binary) code: `editwl-bin rom replace-code -r/--rom=<rom-file> -p/--proc=<processor> -i/--in=<code-bin-file> -o/--out=<new-rom-file>`

    - Example: `editwl-bin rom replace-code --rom=rom.nds --proc=arm7 --in=arm7.bin --out=new.nds`

- Replace (binary) codes: `editwl-bin rom replace-codes -r/--rom=<rom-file> -7/--in7=<code7-bin-file> -9/--in9=<code9-bin-file> -o/--out=<new-rom-file>`

    - Example: `editwl-bin rom replace-codes --rom=rom.nds -7 arm7.bin --in9=arm9.bin -o new.nds`

This are brief descriptions of what each command does, check the help subcommand for each main subcommand for more info: `editwl-bin <cmd> -h/--help`, like `editwl-bin bmg -h` or `editwl-bin rom --help`.

## Building

### libnedit

These libraries have basically no dependencies (other than the standard) and can easily be embedded in any C/C++ project.

### editwl-bin

This project is built using make, CMake and Qt:

```sh
mkdir build
cd build
cmake ..
make
```

## TODO

### libeditwl

- Migrate code from [libnedit](https://github.com/XorTroll/NitroEdit):

  - NCGR

  - NCLR

  - NSCR

  - SBNK

  - SDAT

  - SSEQ

  - STRM

  - SWAR

- Implement saving texture as NCGR+NCLR+NSCR

- Implement saving in utility.bin files

- Support other formats within SDATs (STRM, SSEQ, etc.)

- Models and model textures (NSBMD, NSBTX)

- Support for remaining BMG message encodings

- For multiple palette NCGR+NCLR textures, allow choosing the palette to load

- Support ignored attributes in NSCR data (check the links credited below)

- Support PMCP section in NCLRs

## Support

Any suggestions, ideas and contributions are always welcome ;)

## Credits

- Some already existing PC ROM editors were really helpful in order to understand several file formats, and as the base for this PC editor: [Every File Explorer](https://github.com/Gericom/EveryFileExplorer), [NSMBe5](https://github.com/Dirbaio/NSMB-Editor), [MKDS Course Modifier](https://www.romhacking.net/utilities/1285/) and [DS Sound Studio](https://dswiki.garhoogin.com/page.jsp?name=DS%20Sound%20Studio)

- The following web pages were also really helpful in order to understand several file formats:
  - https://www.romhacking.net/documents/%5b469%5dnds_formats.htm
  - http://www.feshrine.net/hacking/doc/nds-sdat.html
  - http://problemkaputt.de/gbatek.htm

- The [nintendo-lz](https://gitlab.com/DarkKirb/nintendo-lz) Rust crate was really helpful in order to understand and implement LZ10/LZ11 compression formats in C++.

- `editwl-bin` uses [args](https://github.com/Taywee/args) C++ libraries to parse command-line arguments.
