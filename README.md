# file-freshener
Update your fies once they have been modified

There's not too much to this utility so far.

You add files like: `file-freshener -insert <filename> [-dangerous]?`. The
dangerous flag tells the utility to make sure that you are prompted when you
invoke the freshen command(this might be good for system configuration files in
a git repo or something).

You are allowed to do this with the `-insert <filename> [-dangerous]?` multiple
times in an invokation. This holds true with all of the commands so far.

To remove a file from updating (if you like your current configuration or
something), then you can use `-remove <filename>`. This command does not inform
you if it did not actually have a file to delete for that. As such, it behooves
you to be careful with it(as you should always be when deleting data).

To actually update the files from their various sources run it with `-freshen
[-safe-only]?`. This will prompt you (if you don't feed it `-safe-only`, in which case it will skip them) on files that you have marked as dangerous.

If you should need to change a destination file's source path, then that may be done with `-replace <destination> <new-source>`.

Due to the "lovely" way that this stores its information, invoking it with admin privelages may not work at all, or may end up creating a nice user directory for root with a "freshen.db" in it.
