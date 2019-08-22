Configuration changes
---------------------

Error checking is stricter for wallet `-avoidpartialspends`, `-disablewallet`, `-rescan`, `-salvagewallet`, `-spendzeroconfchange`, `-walletbroadcast`, `-walletrbf`, `-keypool`, `-txconfirmtarget`, `-dblogsize`, `-addresstype`, `-changetype`, `-wallet`, and `-walletnotify` settings. Values that were previously ambiguous or ignored or would trigger warnings or error later at runtime will now trigger errors immediately on startup.

Specifying `-nowalletnotify` now disables wallet notifications instead of trying to execute the string "0" as a shell command.
