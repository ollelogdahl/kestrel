Bugs
====

- After allocing bo:s, we need to wait for TLB updates i believe.
  Sometimes, executing a command list directly after may cause
  page faults, as the address has not yet been fixed.
