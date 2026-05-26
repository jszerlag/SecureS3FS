To compile this stand-alone AES tool, first navigate to this folder

run "gcc AES.c -o AES -lcrypto -lssl"

You will now be able to run the program using "./AES -e/d -p password -in infile -out -outfile (-nosalt)"

-e is selected for encryption, -d is selected for decryption

if you run the file using "./AES" a help command will be shown to you breaking down the input.