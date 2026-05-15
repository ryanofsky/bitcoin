Logging
-------

- The `-loglevel` option can now be used standalone without also specifying
  `-debug`. A global level such as `-loglevel=debug` enables all log
  categories at that level (equivalent to `-debug=1 -loglevel=debug`). A
  per-category entry such as `-loglevel=net:trace` enables that category
  (equivalent to `-debug=net -loglevel=net:trace`). Comma-separated entries
  in a single argument are also supported, e.g. `-loglevel=debug,net:trace`.
  The three logging options now always follow a fixed precedence order
  regardless of where they appear on the command line or in config files:
  `-debug` is applied first (lowest precedence), then `-loglevel`, then
  `-debugexclude` (highest precedence).

- Fixed a bug where `-loglevel=qt:debug` was silently ignored. Log categories
  with short names (such as `qt`, which is two characters) were incorrectly
  treated as invalid global level strings and rejected without an error message.

- Categories enabled at runtime via the `logging` RPC are now always enabled
  at debug level, regardless of whether `-loglevel=trace` was set at startup.
  In practice this only matters if you use the `logging` RPC to enable a
  category that has trace-level messages you want to see, in which case you
  will still need to start the node with `-loglevel=<category>:trace`.
