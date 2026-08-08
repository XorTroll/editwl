# Common formats

This page covers common file structures/sections used by several documented DS file formats.

## Common file header

Many file formats start with the following header structure:

| Offset | Size | Description             |
|--------|------|-------------------------|
| 0x00   | 0x04 | File magic              |
| 0x04   | 0x02 | Byte order              |
| 0x06   | 0x02 | Version                 |
| 0x08   | 0x04 | Total file size         |
| 0x0C   | 0x02 | This header size (0x10) |
| 0x0E   | 0x02 | Section count           |

## Common section header

Many formats contain sections starting with the following header structure:

| Offset | Size | Description      |
|--------|------|------------------|
| 0x00   | 0x04 | Block magic      |
| 0x04   | 0x04 | Total block size |

Games typically make use of a dedicated function for finding a given block. This function will keep iterating through the existing blocks of the file data, until a block is found whose magic matches the desired block magic.
