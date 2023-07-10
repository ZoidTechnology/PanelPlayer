# Colorlight Protocol
The Colorlight protocol consists of layer-2 ethernet frames. The header doesn't include the EtherType field. In its place is a single byte indicating packet type. When this byte is less than 0x06 the frame will be invalid and may be dropped by some network controllers. The receiving card uses a MAC address of `11:22:33:44:55:66`. The sender uses a MAC address of `22:22:33:44:55:66`. The following packets have been identified:

## Display Frame (0x01)
Display the stored image. Sending frames faster than around 50 Hz results in stuttering. The byte at offset 6 appears to be incremented by the receiving card.

| Offset | Length | Description      |
|--------|--------|------------------|
| 0      | 1      | Always 0x07      |
| 1      | 21     | Always 0x00      |
| 22     | 1      | Brightness       |
| 23     | 1      | Always 0x05      |
| 24     | 1      | Always 0x00      |
| 25     | 1      | Red brightness   |
| 26     | 1      | Green brightness |
| 27     | 1      | Blue brightness  |
| 28     | 71     | Always 0x00      |

## Set Brightness (0x0A)
Set brightness and colour balance. Brightness values are non-linear. This packet doesn't appear to be required.

| Offset | Length | Description      |
|--------|--------|------------------|
| 0      | 1      | Red brightness   |
| 1      | 1      | Green brightness |
| 2      | 1      | Blue brightness  |
| 3      | 1      | Always 0xFF      |
| 4      | 60     | Always 0x00      |

## Image Data (0x55)
Send a row of image data. Data is split between multiple packets using the horizontal offset field when sending more than 497 pixels to ensure the packet never exceeds the maximum size of an ethernet frame.

| Offset | Length          | Description             |
|--------|-----------------|-------------------------|
| 0      | 2               | Row number              |
| 2      | 2               | Horizontal offset       |
| 4      | 2               | Pixel count             |
| 6      | 1               | Always 0x08             |
| 7      | 1               | Always 0x88             |
| 8      | Pixel count × 3 | Image data in BGR order |