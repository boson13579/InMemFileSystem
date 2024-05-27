#include <iostream>
#include <openssl/aes.h>
#include <openssl/rand.h>
int main(){
    
    unsigned char tmpkey[32];
    RAND_bytes(tmpkey, sizeof(tmpkey));
    for(int i = 0; i < 32; i++) printf("%02x", tmpkey[i]);
    
    return 0;
}