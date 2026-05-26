Logging
-------

- The `logging` RPC now only toggles between `info` and `debug` log levels,
  instead of remembering previously assigned log levels (which could be
  confusing). This change is only observable when `logging` calls are combined
  with `-loglevel=<category>:<level>` assignments. The `-loglevel` setting also
  continues to work as before when `logging` is not called at runtime.
