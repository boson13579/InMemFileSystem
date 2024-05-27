#include "Encryption.h"
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>


using namespace std;
int main() {
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    string plaintext, skey;
    getline(cin, plaintext);
    vector<unsigned char> content(plaintext.begin(), plaintext.end());

    skey = getpass("Enter key without blank: ");
    cerr << "Key is:" << skey << endl;
    if (skey.size() != 64) return -EAGAIN;
    for (auto i : skey) {
        if (!isdigit(i) && !(i >= 'a' && i <= 'f'))
            return -EAGAIN;
    }

    unsigned char key[32], iv[16];
    for (int i = 0; i < 64; i += 2) {
        std::string byteString = skey.substr(i, 2);
        key[i/2] = (unsigned char) std::stoul(byteString, nullptr, 16);
    }

    RAND_bytes(iv, sizeof(iv));

    unsigned char ciphertext[4096];
    unsigned char decryptedtext[4096];

    int ciphertext_len = encrypt(content.data(), content.size(), key, iv, ciphertext);
    int decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';

    cout << "Plaintext is:" << endl;
    for (int i = 0; i < content.size(); i++) printf("%02x ", content[i]);
    cout << endl;

    cout << "Ciphertext is:" << endl;
    for (int i = 0; i < ciphertext_len; i++) printf("%02x ", ciphertext[i]);
    cout << endl;

    cout << "Decrypted text is:" << endl;
    for (int i = 0; i < decryptedtext_len; i++) printf("%02x ", decryptedtext[i]);
    cout << endl;

    return 0;
}
