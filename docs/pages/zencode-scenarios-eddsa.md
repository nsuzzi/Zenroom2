
# EdDSA

[EdDSA](https://en.wikipedia.org/wiki/EdDSA)(*Edwards-curve Digital Signature Algorithm*) is a variant of Schnorr signature algorithm. The main change apported in this scheme is the employment of twisted Edwards-curves that result in having higher perfomance with respect to other digital signature algorithms, wihtout sacrificing security. Moreover this scheme resolve also another major issue emerged in the other signature schemes: it **does not need a criptographically secure random number generator** to create a random signature.

In Zenroom, using the *Scenario eddsa*, you will use the **Ed25519** version of this algorithm that is based on *SHA-512* and the *Edwards form of Curve25519*. Moreover, the private key, the public key and the signature generated with Zenroom will be encoded in base58.

# Key Generation

## Private key

The script below generates a **EdDSA** private key.

[](../_media/examples/zencode_cookbook/eddsa/alice_keygen.zen ':include :type=code gherkin')

The output should look like this:

[](../_media/examples/zencode_cookbook/eddsa/alice_keys.json ':include :type=code json')

### Upload a private key

Key generation in Zenroom uses by default a pseudo-random as seed, that is internally generated.

You can also opt to use a seed generated elsewhere, for example by using the [keypairoom](https://github.com/ledgerproject/keypairoom) library or it's [npm package](https://www.npmjs.com/package/keypair-lib). Suppose you end with a **EdDSA private key**, like:

[](../_media/examples/zencode_cookbook/eddsa/secret_key.json ':include :type=code json')

Then you can upload it with a script that look like the following script:

[](../_media/examples/zencode_cookbook/eddsa/alice_key_upload.zen ':include :type=code gherkin')

Here we simply print the *keyring*.

## Public key

Once you have created a private key, you can feed it to the following script to generate the **public key**:

[](../_media/examples/zencode_cookbook/eddsa/alice_pubkey.zen ':include :type=code gherkin')

The output should look like this:

[](../_media/examples/zencode_cookbook/eddsa/alice_pubkey.json ':include :type=code json')

# Signature

In this example we'll sign three objects: a string, a string array and a string dictionary, that we'll verify in the next script. Along with the data to be signed, we'll need the private key. The private key is in the file we have generated with the first script, while the one with the messages that we will sign is the following:

[](../_media/examples/zencode_cookbook/eddsa/message.json ':include :type=code json')

The script to **sign** these objects look like this:

[](../_media/examples/zencode_cookbook/eddsa/sign_from_alice.zen ':include :type=code gherkin')

And the output should look like this:

[](../_media/examples/zencode_cookbook/eddsa/signed_from_alice.json ':include :type=code json')

# Verification

In this section we will **verify** the signatures produced in the previous step. to carry out this task we would need the signatures, the messages and the signer public key. The signatures and the messages are contanined in the output of the last script, while the signer public key can be found in the output of the second script. So the input files should look like:

[](../_media/examples/zencode_cookbook/eddsa/signed_from_alice.json ':include :type=code json')


[](../_media/examples/zencode_cookbook/eddsa/alice_pubkey.json ':include :type=code json')

The script to verify these signatures is the following:

[](../_media/examples/zencode_cookbook/eddsa/verify_from_alice.zen ':include :type=code gherkin')

The result should look like:

[](../_media/examples/zencode_cookbook/eddsa/verified_from_alice.json ':include :type=code json')

# The script used to create the material in this page

All the smart contracts and the data you see in this page are generated by the scripts [generic_eddsa.bats](https://github.com/dyne/Zenroom/blob/master/test/zencode/generic_eddsa.bats). If you want to run the scripts (on Linux) you should: 
 - *git clone https://github.com/dyne/Zenroom.git*
 - install  **jq**
 - download a [zenroom binary](https://zenroom.org/#downloads) and place it */bin* or */usr/bin* or in *./Zenroom/src*
