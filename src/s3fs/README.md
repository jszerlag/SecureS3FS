Included is the fdcachce_entity.cpp file

This is the only file I have changed, other than changing line 136 from false to true in curl.cpp
if there are any issues with hashing or unsigned payloads, I would recommend changing this to "true"

To enter a password file: go to line 100 of the fdcache_entity.cpp file and write the path for your file into the #define PASSWORD_FILE
I apologize but I could not figure out how to allow the user to enter a file location through the mounting procedure.

Insert the changed fdcache_entity file into s3fs, and run meake clean on the s3fs folder.

then run "./autogen.sh"
and "./configure"

you can then run "make"

This should have installed s3fs, you can check by doing s3fs --version

create a directory for the mount to use
(mkdir /dir/)

create the the file .passwd-s3fs in the root directory, enter your "AWSkey:SecretKey" in this format

now you can mount using the command "s3fs bucket-name mounting-point -o passwd_file=~/.passwd-s3fs"

files should be able to be seen in the online version of s3, and files should be encrypted or decrypted using the supplied password file.

