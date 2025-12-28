Architecture
============

Each driver library (`libkestrel_<platform>.so`) exports only
a single symbol.

.. doxygenfile:: kestrel/interface.h

When a new Device is created, the Kestrel runtime iterates
the available DRM devices on the system, and tries to resolve
a suitable driver library. The symbol is resolved and bind
all functions to that driver.
