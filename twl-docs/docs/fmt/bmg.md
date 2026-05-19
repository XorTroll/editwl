# BMG format

BMG (likely standing for "binary message"/"basic message") is a format used by several DS games (although only in first-party Nintendo games?).

> Note: the format is also used in Wii games (and apparently GameCube as well), here only the format found in DS games is described. Check [here](https://wiki.tockdom.com/wiki/BMG_(File_Format)) for more Wii-related details of the format.

- [BMG format](#bmg-format)
  - [`INF1` section](#inf1-section)
    - [Message entry](#message-entry)
  - [`DAT1` section](#dat1-section)
    - [Message data](#message-data)
    - [Escape sequences](#escape-sequences)
  - [`MID1` section](#mid1-section)
  - [Games using the format](#games-using-the-format)
    - [Mario Kart DS](#mario-kart-ds)
    - [Super Mario 64 DS](#super-mario-64-ds)
    - [Animal Crossing: Wild World](#animal-crossing-wild-world)
    - [DSi Menu (launcher)](#dsi-menu-launcher)
    - [Practise English!](#practise-english)
    - [Download Play data found in many games (utility.bin)](#download-play-data-found-in-many-games-utilitybin)
    - [Nintendogs DS](#nintendogs-ds)
    - [New Super Mario Bros.](#new-super-mario-bros)
    - [WarioWare D.I.Y](#warioware-diy)
    - [Chibi-Robo! Park Patrol](#chibi-robo-park-patrol)
    - [Custom Robo Arena](#custom-robo-arena)
    - [DS Rakubiki Jiten](#ds-rakubiki-jiten)

BMGs start like usual DS formats, containing a slightly different [common header](common.md):

| Offset | Size | Description                                                       |
|--------|------|-------------------------------------------------------------------|
| 0x00   | 0x08 | Magic (0x31676D624753454D = "MESGbmg1")                           |
| 0x08   | 0x04 | Total file size                                                   |
| 0x0C   | 0x04 | Section count                                                     |
| 0x10   | 0x01 | Text encoding (CP-1252 = 1, UTF-16 = 2, Shift JIS = 3, UTF-8 = 4) |
| 0x11   | 0x1  | Unknown (usually zero)                                            |
| 0x12   | 0x2  | Unknown (usually zero)                                            |
| 0x14   | 0x4  | Unknown (usually zero)                                            |
| 0x18   | 0x4  | Unknown (usually zero)                                            |
| 0x1C   | 0x4  | Unknown (usually zero)                                            |

All BMG files seem to have `INF1` and `DAT1` sections, optionally having a `MID1` section (thus section count is 2 or higher).

The header is followed by the following sections (in order, in case they are present):

## `INF1` section

This section starts with a [common header](common.md#common-section-header) and has the following structure:

| Offset | Size                               | Description                       |
|--------|------------------------------------|-----------------------------------|
| 0x00   | 0x04                               | Block magic (0x31464E49 = "INF1") |
| 0x04   | 0x04                               | Total block size                  |
| 0x08   | 0x02                               | Message count                     |
| 0x0A   | 0x02                               | Message entry size                |
| 0x0C   | 0x04                               | File ID                           |
| 0x10   | Message count * Message entry size | Message entries                   |

### Message entry

Message entries have the following structure:

| Offset | Size                      | Description           |
|--------|---------------------------|-----------------------|
| 0x00   | 0x04                      | Message data offset   |
| 0x04   | Message entry size - 0x04 | Additional attributes |

The message data offset is an offset relative to past `DAT1` section header (thus relative to the start of the section data).

In general games have no additional attributes in messages, hence message entries typically only contain the message data offset.

This section has (always?) end zero-byte padding to be 0x20-aligned.

## `DAT1` section

This section starts with a [common header](common.md#common-section-header) and has the following structure:

| Offset | Size     | Description                       |
|--------|----------|-----------------------------------|
| 0x00   | 0x04     | Block magic (0x31544144 = "DAT1") |
| 0x04   | 0x04     | Total block size                  |
| 0x08   | Variable | Message data                      |

This section also has (always?) end zero-byte padding to be 0x20-aligned.

### Message data

Message content has two kinds of possible data: plain text and escape sequences used to encode binary data.

Escape sequences (at least for UTF-8 and UTF-16, unknown for the others) have the following structure:

| Offset                | Size                                        | Description               |
|-----------------------|---------------------------------------------|---------------------------|
| 0x00                  | Character size (1 = UTF-8, 2 = UTF-16, etc) | Escape character '\u001A' |
| Character size        | 0x01                                        | Total sequence size       |
| Character size + 0x01 | Total sequence size - Character size - 0x01 | Encoded bytes             |

Every character read outside of escape sequences is treated as plain message text. These are read until a null character is found, which indicates the end of the message data.

For example, message data `41 00 41 00 1A 00 06 12 34 56 53 00 53 00 00 00` corresponds to an UTF-16 message with: plain text "AA", escape with bytes `12 34 56`, plain text "BB" (finishing with the corresponding null character).

### Escape sequences

Escape sequences are used for special text formatting. Common use cases are changing text color, formatting in-game strings...

Sequences differ between encodings: sequence formats in UTF-8 and UTF-16 BMG files 

## `MID1` section

This section starts with a [common header](common.md#common-section-header) and has the following structure:

| Offset | Size                    | Description                       |
|--------|-------------------------|-----------------------------------|
| 0x00   | 0x04                    | Block magic (0x3144494D = "MID1") |
| 0x04   | 0x04                    | Total block size                  |
| 0x08   | 0x02                    | Message ID count                  |
| 0x0A   | 0x01                    | Unknown                           |
| 0x0B   | 0x01                    | Unknown                           |
| 0x0C   | 0x04                    | Unknown                           |
| 0x10   | 0x04 * Message ID count | Message IDs                       |

This section also has (always?) end zero-byte padding to be 0x20-aligned.

The message ID count is the same as the message count in the previous `INF1` section. IDs are apparently used in games with multiple BMG files, where the same message is present in several BMG files, where the ID is probably used to access the message (since the message is probably in different indexes in the data section).

## Games using the format

While it appears to be a standard format, unlike with other SDK formats, no two games have the same code for loading BMG files. It looks like the SDK does not provide code, so each game developers had to parse the files themselves (either that or the SDK code changed completely game after game, which is highly unlikely).

Note that this list is not fully exhaustive, as only a handful of games have been searched, but the explored coves cover enough variety to show the half-standardized nature of this format.

### Mario Kart DS

Found files:

- `/data/CharacterKartSelect_<lang>.carc/kart_select.bmg`

- `/data/Main2D_<lang>.carc/common.bmg`

- `/data/Static2D.carc/MBChild_<lang>.bmg`

- `/data/Scene/Emblem_<lang>.carc/emblem.bmg`

- `/data/Scene/Ghost_<lang>.carc/ghost.bmg`

- `/data/Scene/Menu_<lang>.carc/*.bmg`

- `/data/Scene/MenuDL_<lang>.carc/*.bmg`

- `/data/Scene/Option_<lang>.carc/option.bmg`

- `/data/Scene/Record_<lang>.carc/record.bmg`

- `/data/Scene/Result_<lang>.carc/secret.bmg`

- `/data/Scene/StaffRoll.carc/staffRoll.bmg`

- `/data/Scene/StaffRoll_<lang>.carc/staffRoll.bmg`

- `/data/Scene/Title_<lang>.carc/title.bmg`

- `/data/Scene/WiFiMenu_<lang>.carc/wifi.bmg`

- `/data/Scene/WLMenu_<lang>.carc/banner.bmg`

  - Used encoding: UTF-16

  - Found sections: `INF1` + `DAT1`

### Super Mario 64 DS

- `/data/message/msg_data_<lang>.bin`

  - Used encoding: CP1252 (according to header), supposedly is Shift-JIS in reality...?
  
  - Found sections: `INF1` + `DAT1`

  - LZ77-compressed files
  
  - Magics are endian-swapped! (`1FNI`, `1TAD`, etc)

  - Has a final file padding of 0xFF

### Animal Crossing: Wild World

- `/script/<lang>/<...>/*.bmg`

  - Used encoding: UTF-8

  - Found sections: `INF1` + `DAT1`

### DSi Menu (launcher)

- `/message/ww/<lang>/menu_common.bmg`

  - Used encoding: UTF-16

  - Found sections: `INF1` + `DAT1`

### Practise English!

- `/common/emsg/<...>/*.bmg`

- `/common/tmsg/<...>/*.bmg`

- `/<lang>/emsg/<...>/*.bmg`

- `/<lang>/tmsg/<...>/*.bmg`

  - Used encoding: UTF-8

  - Found sections: `INF1` + `DAT1` + `STR1` (custom section?) (`DAT1` is empty while `STR1` has the actual message strings...?)

  - Uses a variant format:

    - `INF1` section entries are of size 0x10 and contain the following:

    | Offset | Size                    | Description                                                                   |
    |--------|-------------------------|-------------------------------------------------------------------------------|
    | 0x00   | 0x04                    | Message index (not an offset!)                                                |
    | 0x04   | 0x02                    | Start offset relative to STR1 section + 0x8 (skipping magic and section size) |
    | 0x06   | 0x02                    | End offset relative to STR1 section + 0x8                                     |
    | 0x08   | 0x02                    | Previous end offset + 1 (?)                                                   |
    | 0x0A   | 0x01                    | Unknown                                                                       |
    | 0x0B   | 0x01                    | Unknown                                                                       |
    | 0x0C   | 0x01                    | Unknown                                                                       |
    | 0x0D   | 0x03                    | Unknown                                                                       |

    - STR1 seems to be made of:

    | Offset | Size                    | Description         |
    |--------|-------------------------|---------------------|
    | 0x00   | 0x04                    | Magic               |
    | 0x04   | 0x04                    | Section size        |
    | 0x08   | Variable                | Message string data |

    - String data appears to be made of NULL-terminated strings (presumably so they can be directly read as a valid `char*` in code).

    - An unused byte is left after the header section, so that strings begin at `STR1` + 0x9.

### Download Play data found in many games (utility.bin)

- `/msg/<lang>-bmg.l`

  - Used encoding: UTF-16

  - Found sections: `INF1` + `DAT1`

  - LZ77-compressed files

### Nintendogs DS

- `/Message/<lang>/*.bmg`

  - Used encoding: UTF-16

  - Found sections: `INF1`, `DAT1`, `MID1`, `FLI1` (only large ones?), `FLW1` (only large ones?)

### New Super Mario Bros.

- `/script/data.bmg.<lang>`, `/script/game.bmg.<lang>`, `/script/course.bmg.<lang>`

  - Used encoding: UTF-16

  - Found sections: `INF1`, `DAT1`

### WarioWare D.I.Y

- `/E/Data/Mesg*.bmg` 

  - Used encoding: UTF-8

  - Found sections: `INF1`, `DAT1` (loading code only checks for these two sections anyway)

### Chibi-Robo! Park Patrol

Found files:

- `/Msg*.bmg`

  - Used encoding: UTF-8

  - Found sections: `INF1`, `DAT1`

  - Uses C-like formatting instead of escape sequences (literals like `%02d` directly in strings)

### Custom Robo Arena

- `/msg/*.cbmg`

  - Used encoding: CP1252

  - Found sections: `INF1`, `DAT1`

  - Magics are endian-swapped! (`1FNI`, `1TAD`, etc)

### DS Rakubiki Jiten

Found files:

- `/message/iplmsg_<lang>.bmg`

  - Used encoding: UTF-8

  - Found sections: `INF1`, `DAT1`
