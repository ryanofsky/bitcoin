RPC
---
- Categories enabled at runtime via the `logging` RPC are now always enabled
  at `debug` level, even if `-loglevel=trace` was set at startup.
  A new `loglevels` RPC can be used to enable `trace` logging at runtime.

- The `logging` RPC now checks category names earlier, so if any invalid
  categories are specified, no changes will be applied, rather than being
  partially applied.

- A new `loglevels` RPC has been added, which provides a superset of
  functionality of the `logging` RPC, and allows enabling `trace` logs as well
  as `debug` logs. When called without arguments, it returns all categories with
  their current level. When called with a `{category: level}` dictionary, it sets
  the specified levels and returns the updated state. The special key `"all"`
  sets the level for every category at once; when combined with per-category
  keys, `"all"` is applied first so individual overrides take effect.

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
