#include "Encryption.h"

#include <iostream>
#include <vector>

using namespace std;

void handleErrors(void) {
    ERR_print_errors_fp(stderr);
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) handleErrors();
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) handleErrors();
    ciphertext_len = len;
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) handleErrors();
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) handleErrors();
    plaintext_len = len;
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

unsigned char *getkey() {
    string skey;

    bool rsize = false, rchar = false;
    while(!rsize || !rchar) {
        skey = getpass("Enter key without blank: ");

        if (skey.size() != 64) {
            cout << "Key must be 32 bytes long." << endl;
            rsize = false;
        } else {
            rsize = true;
        }

        for (int i = 0; i < 64; i++) {
            if (!isdigit(skey[i]) && !(skey[i] >= 'a' && skey[i] <= 'f')){
                cout << "Key must be in hexadecimal format." << endl;
                rchar = false;
                break;
            } else {
                rchar = true;
            }
        }
    }

    unsigned char *key = (unsigned char *)malloc(32);
    for (int i = 0; i < 64; i += 2) {
        std::string byteString = skey.substr(i, 2);
        key[i / 2] = (unsigned char)std::stoul(byteString, nullptr, 16);
    }

    return key;
}
