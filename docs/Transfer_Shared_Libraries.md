This guide shows to to transfer the precompiled libraries found in the [lib/ directory](../lib/) to the EVAcharge_SE board.

1. Transfer the contents of the lib directory to the EVAcharge SE. This can be done by using an SFTP client such as FileZilla, where files can be dragged and dropped, or via a command like tool like SCP.
It is recommended to transfer the entire directory structure.

2. Once the files are transferred to the EVAcharge_SE, they can be moved to the `/usr/` directory by using the `rsync` tool. First, do a dry-run of the copy to ensure non of the system directory structures are being changed or files being deleted. This can be done for the wolfSSL libraries by running the following:
```
rsync -ruvni wolfssl/ /usr/
rsync -ruvni wolfmqtt/ /usr/
```
Here, `wolfmqtt` is the name of the source directory in which the libraries and header files are located in a proper structure (in separate folders such as bin, include, lib, and share), and the destination is `/usr/`. The trailing `/` after the source and destination directories are important to ensure that the contents of the directories are copied rather than the directories themselves. 

The `-r` option tells rsync to copy the directories recursively, the `-u` option tells rsync to only update files that are newer in the source directory, the `-v` option will show verbose output, so you can see which files are being considered for transfer, the `-n` option is the dry-run option which will simulate the transfer and display what it would do, and the `-i` option causes `rsync` to output a detailed itemization of the proposed transfer, including whether files will be added, updated, deleted, and where they will be copied to in the directory. 

3. Once you have reviewed the output of the dry-run and are satisfied with the actions `rsync` will take, the actual transfer can be performed by running the following command: 
```
sudo rsync -ruv wolfssl/ /usr/
sudo rsync -ruv wolfmqtt/ /usr/
```
