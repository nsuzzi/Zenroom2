#!/usr/bin/env bash

n_signatures=1000

if [ $# != 1 ]; then
    echo "Generating $n_signatures signatures and verify them.\nIf you want to use a different number use: $0 <number_of_signatures>"
else
    n_signatures=$1
    echo "Generating $n_signatures signatures and verify them."
fi
msg='"message": "I love the Beatles, all but 3",'

echo signing $n_signatures times...
echo '"ethereum_addresses": [' >addresses.txt
echo '"ethereum_signatures": [' >signatures.txt
time for ((i=1; i<=$n_signatures; i++)); do
    zenroom -a key_add_sign_gen.keys.bench -z key_add_sign_gen_manual.zen.bench >res_1.json 2>execution_data_1.txt
    if [ "$?" != "0" ]; then
        echo "Error during generation"
        cat execution_data_1.txt
        exit 1
    fi
    if [ "$i" != "$n_signatures" ]; then
        cat res_1.json | cut -d, -f1 | cut -d: -f2 | sed 's/$/,/' >> addresses.txt
        cat res_1.json | cut -d, -f2 | cut -d: -f2 | cut -d\} -f1 | sed 's/$/,/' >> signatures.txt
    else
        cat res_1.json | cut -d, -f1 | cut -d: -f2 >> addresses.txt
        cat res_1.json | cut -d, -f2 | cut -d: -f2 | cut -d\} -f1 >> signatures.txt
    fi
done
echo '],' >>addresses.txt
echo ']' >>signatures.txt
echo "{$msg" > add_and_sign.json
cat addresses.txt >> add_and_sign.json
cat signatures.txt >> add_and_sign.json
echo "}" >> add_and_sign.json
rm res_1.json execution_data_1.txt addresses.txt signatures.txt

echo
echo verifying $n_signatures signatures...
time zenroom -a add_and_sign.json -z batch_verification.zen.bench >res_2.json 2>execution_data_2.txt
if [ "`cat res_2.json`" != "{\"output\":[\"OK\"]}" ]; then
    echo "Error during verification"
    cat execution_data_2.txt
    exit 0
fi
rm -f res_2.json execution_data_2.txt add_and_sign.json
