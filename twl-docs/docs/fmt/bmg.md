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
