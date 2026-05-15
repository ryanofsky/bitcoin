Logging
-------

- Fixed a bug where `-loglevel=qt:debug` was silently ignored. Log categories
  with short names (such as `qt`, which is two characters) were incorrectly
  treated as invalid global level strings and rejected without an error message.

- Categories enabled at runtime via the `logging` RPC are now always enabled
  at debug level, regardless of whether `-loglevel=trace` was set at startup.
  In practice this only matters if you use the `logging` RPC to enable a
  category that has trace-level messages you want to see, in which case you
  will still need to start the node with `-loglevel=<category>:trace`.
