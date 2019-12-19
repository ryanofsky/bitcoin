Configuration
-------------

Handling of negated `-noseednode`, `-nobind`, `-nowhitebind`, `-norpcbind`, `-norpcallowip`, `-norpcwhitelist`, `-notest`, `-noasmap`, `-norpcwallet`, `-noonlynet`, and `-noexternalip` options has changed. Previously negating these options had various confusing and undocumented side effects. No negating them just resets them and restores default behaviors, as if the options were not specified at all.
