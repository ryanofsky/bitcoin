macOS
==

1. Download from www.codesynthesis.com/products/odb/download.xhtml: 
 + odb-2.4.0-i686-macosx binary
 + the common runtime library libodb-2.4.0
 + the database runtime libodb-pgsql-2.4.0 (we will add other databases after prototyping) 
 + optionally the examples odb-examples-2.4.0 

2. Decompress and move the resulting folders to a more permanent folder than Downloads.

3. In your shell config (zshrc in my case), add the ODB bin subdirectory to your path, like so <code>export PATH=/Users/pierrerochard/src/odb-2.4.0-i686-macosx/bin:$HOME/.cargo/bin:$HOME/bin:/usr/local/bin:$PATH</code>

4. In libodb run the usual <code>./configure && make && make install</code>

5. If you want to install the examples configure with these flags <code>./configure --with-database=pgsql CXXFLAGS=-std=c++0x </code>
