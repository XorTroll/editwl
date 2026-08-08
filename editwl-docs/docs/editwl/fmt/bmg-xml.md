# Custom BMG-XML format

`editwl` uses a custom XML specification to convert BMG message files from/to readable and editable messages.

## Description

This is a simple BMG XML example:

```xml
<!DOCTYPE xml>
<bmg encoding="UTF-16">
    <message>Hello there!</message>
    <message>This format <escape data="ff 00"/>rules<escape data="ff 01"/>!
BMG supremacy</message>
</bmg>
```

The XML file starts with a root `bmg` element which contains a `encoding` field. Do not confuse this encoding with the usual XML encoding, since this field contains the encoding which will be followed when converting it to a BMG binary. Possible values are `utf-16`, `utf-8`, `shift jis` and `cp-1252` (uppercase/lowercase and/or without the middle hyphens).

BMGs may make use of the file ID value, which can be specified as an optional `id` field containing a decimal integer. This value is treated as `0` otherwise.

All children inside are `message` elements, whose contents will be plain text or escape sequences, represented by an `escape` element containing binary hex values in a `data` field.

Keep in mind that tab/indenting is encoded within the message, thus forcing multiline messages to be like the second message in the example.

For BMGs containing extra attributes or message IDs, these are represented as optional message fields:

```xml
<!DOCTYPE xml>
<bmg encoding="UTF-16">
    <message id="0" attributes="00 11 22">Hello there!</message>
    <message id="12" attributes="AA BB CC">This format <escape data="ff 00"/>rules<escape data="ff 01"/>!
BMG supremacy</message>
</bmg>
```

If message IDs are used, all messages must have an ID field. Same thing applies separately for attributes, where all attribute data must be the same size for all messages.
