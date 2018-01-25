USB support is implemented using the STM32 USB Device library.

The library is copied into the core project to make customizations easier to maintain
across different ports. The USBD library supports all versions of the HAL and LL.

GEX uses USB classes CDC/ACM and MSC/SCSI.
The two classes are combined into a composite class with association descriptors.

USB interrupts are processed by the USBD library and endpoint callbacks in the composite
class are fired. To avoid race conditions (and because DAPlink did it the same way), the
events are notified to the USB thread (TaskMain) which calls endpoint handlers in the
corresponding class drivers.

VFS is handled synchronously on the main thread. CDC messages (TinyFrame data) are queued
and processed by the message queue thread. This makes it possible to query hardware
(e.g. slow USART or NeoPixel) without stalling the USB communication. This arrangement
also makes it possible to wait on a binary semaphore when sending data back to host. The
semaphore is set from the CDC TxComplete callback and taken by the TinyFrame write
function, serving as a form of flow control.
