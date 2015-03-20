/* apps/progs.h */
/* automatically generated by progs.pl for openssl.c */

extern int verify_main(int argc, char *argv[]);
extern int asn1parse_main(int argc, char *argv[]);
extern int req_main(int argc, char *argv[]);
extern int dgst_main(int argc, char *argv[]);
extern int dh_main(int argc, char *argv[]);
extern int dhparam_main(int argc, char *argv[]);
extern int enc_main(int argc, char *argv[]);
extern int passwd_main(int argc, char *argv[]);
extern int gendh_main(int argc, char *argv[]);
extern int errstr_main(int argc, char *argv[]);
extern int ca_main(int argc, char *argv[]);
extern int crl_main(int argc, char *argv[]);
extern int rsa_main(int argc, char *argv[]);
extern int rsautl_main(int argc, char *argv[]);
extern int dsa_main(int argc, char *argv[]);
extern int dsaparam_main(int argc, char *argv[]);
extern int ec_main(int argc, char *argv[]);
extern int ecparam_main(int argc, char *argv[]);
extern int x509_main(int argc, char *argv[]);
extern int genrsa_main(int argc, char *argv[]);
extern int gendsa_main(int argc, char *argv[]);
extern int s_server_main(int argc, char *argv[]);
extern int s_client_main(int argc, char *argv[]);
extern int speed_main(int argc, char *argv[]);
extern int s_time_main(int argc, char *argv[]);
extern int version_main(int argc, char *argv[]);
extern int pkcs7_main(int argc, char *argv[]);
extern int cms_main(int argc, char *argv[]);
extern int crl2pkcs7_main(int argc, char *argv[]);
extern int sess_id_main(int argc, char *argv[]);
extern int ciphers_main(int argc, char *argv[]);
extern int nseq_main(int argc, char *argv[]);
extern int pkcs12_main(int argc, char *argv[]);
extern int pkcs8_main(int argc, char *argv[]);
extern int spkac_main(int argc, char *argv[]);
extern int smime_main(int argc, char *argv[]);
extern int rand_main(int argc, char *argv[]);
extern int engine_main(int argc, char *argv[]);
extern int ocsp_main(int argc, char *argv[]);
extern int prime_main(int argc, char *argv[]);

#define FUNC_TYPE_GENERAL       1
#define FUNC_TYPE_MD            2
#define FUNC_TYPE_CIPHER        3

typedef struct {
    int type;
    const char *name;
    int (*func) (int argc, char *argv[]);
} FUNCTION;

FUNCTION functions[] = {
    {FUNC_TYPE_GENERAL, "verify", verify_main},
    {FUNC_TYPE_GENERAL, "asn1parse", asn1parse_main},
    {FUNC_TYPE_GENERAL, "req", req_main},
    {FUNC_TYPE_GENERAL, "dgst", dgst_main},
#ifndef OPENSSL_NO_DH
    {FUNC_TYPE_GENERAL, "dh", dh_main},
#endif
#ifndef OPENSSL_NO_DH
    {FUNC_TYPE_GENERAL, "dhparam", dhparam_main},
#endif
    {FUNC_TYPE_GENERAL, "enc", enc_main},
    {FUNC_TYPE_GENERAL, "passwd", passwd_main},
#ifndef OPENSSL_NO_DH
    {FUNC_TYPE_GENERAL, "gendh", gendh_main},
#endif
    {FUNC_TYPE_GENERAL, "errstr", errstr_main},
    {FUNC_TYPE_GENERAL, "ca", ca_main},
    {FUNC_TYPE_GENERAL, "crl", crl_main},
#ifndef OPENSSL_NO_RSA
    {FUNC_TYPE_GENERAL, "rsa", rsa_main},
#endif
#ifndef OPENSSL_NO_RSA
    {FUNC_TYPE_GENERAL, "rsautl", rsautl_main},
#endif
#ifndef OPENSSL_NO_DSA
    {FUNC_TYPE_GENERAL, "dsa", dsa_main},
#endif
#ifndef OPENSSL_NO_DSA
    {FUNC_TYPE_GENERAL, "dsaparam", dsaparam_main},
#endif
#ifndef OPENSSL_NO_EC
    {FUNC_TYPE_GENERAL, "ec", ec_main},
#endif
#ifndef OPENSSL_NO_EC
    {FUNC_TYPE_GENERAL, "ecparam", ecparam_main},
#endif
    {FUNC_TYPE_GENERAL, "x509", x509_main},
#ifndef OPENSSL_NO_RSA
    {FUNC_TYPE_GENERAL, "genrsa", genrsa_main},
#endif
#ifndef OPENSSL_NO_DSA
    {FUNC_TYPE_GENERAL, "gendsa", gendsa_main},
#endif
#if !defined(OPENSSL_NO_SOCK) && !(defined(OPENSSL_NO_SSL2) && defined(OPENSSL_NO_SSL3))
    {FUNC_TYPE_GENERAL, "s_server", s_server_main},
#endif
#if !defined(OPENSSL_NO_SOCK) && !(defined(OPENSSL_NO_SSL2) && defined(OPENSSL_NO_SSL3))
    {FUNC_TYPE_GENERAL, "s_client", s_client_main},
#endif
#ifndef OPENSSL_NO_SPEED
    {FUNC_TYPE_GENERAL, "speed", speed_main},
#endif
#if !defined(OPENSSL_NO_SOCK) && !(defined(OPENSSL_NO_SSL2) && defined(OPENSSL_NO_SSL3))
    {FUNC_TYPE_GENERAL, "s_time", s_time_main},
#endif
    {FUNC_TYPE_GENERAL, "version", version_main},
    {FUNC_TYPE_GENERAL, "pkcs7", pkcs7_main},
#ifndef OPENSSL_NO_CMS
    {FUNC_TYPE_GENERAL, "cms", cms_main},
#endif
    {FUNC_TYPE_GENERAL, "crl2pkcs7", crl2pkcs7_main},
    {FUNC_TYPE_GENERAL, "sess_id", sess_id_main},
#if !defined(OPENSSL_NO_SOCK) && !(defined(OPENSSL_NO_SSL2) && defined(OPENSSL_NO_SSL3))
    {FUNC_TYPE_GENERAL, "ciphers", ciphers_main},
#endif
    {FUNC_TYPE_GENERAL, "nseq", nseq_main},
#if !defined(OPENSSL_NO_DES) && !defined(OPENSSL_NO_SHA1)
    {FUNC_TYPE_GENERAL, "pkcs12", pkcs12_main},
#endif
    {FUNC_TYPE_GENERAL, "pkcs8", pkcs8_main},
    {FUNC_TYPE_GENERAL, "spkac", spkac_main},
    {FUNC_TYPE_GENERAL, "smime", smime_main},
    {FUNC_TYPE_GENERAL, "rand", rand_main},
#ifndef OPENSSL_NO_ENGINE
    {FUNC_TYPE_GENERAL, "engine", engine_main},
#endif
    {FUNC_TYPE_GENERAL, "ocsp", ocsp_main},
    {FUNC_TYPE_GENERAL, "prime", prime_main},
#ifndef OPENSSL_NO_MD2
    {FUNC_TYPE_MD, "md2", dgst_main},
#endif
#ifndef OPENSSL_NO_MD4
    {FUNC_TYPE_MD, "md4", dgst_main},
#endif
#ifndef OPENSSL_NO_MD5
    {FUNC_TYPE_MD, "md5", dgst_main},
#endif
#ifndef OPENSSL_NO_SHA
    {FUNC_TYPE_MD, "sha", dgst_main},
#endif
#ifndef OPENSSL_NO_SHA1
    {FUNC_TYPE_MD, "sha1", dgst_main},
#endif
#ifndef OPENSSL_NO_MDC2
    {FUNC_TYPE_MD, "mdc2", dgst_main},
#endif
#ifndef OPENSSL_NO_RMD160
    {FUNC_TYPE_MD, "rmd160", dgst_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-128-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-128-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-192-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-192-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-256-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_AES
    {FUNC_TYPE_CIPHER, "aes-256-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-128-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-128-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-192-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-192-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-256-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_CAMELLIA
    {FUNC_TYPE_CIPHER, "camellia-256-ecb", enc_main},
#endif
    {FUNC_TYPE_CIPHER, "base64", enc_main},
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des3", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "desx", enc_main},
#endif
#ifndef OPENSSL_NO_IDEA
    {FUNC_TYPE_CIPHER, "idea", enc_main},
#endif
#ifndef OPENSSL_NO_SEED
    {FUNC_TYPE_CIPHER, "seed", enc_main},
#endif
#ifndef OPENSSL_NO_RC4
    {FUNC_TYPE_CIPHER, "rc4", enc_main},
#endif
#ifndef OPENSSL_NO_RC4
    {FUNC_TYPE_CIPHER, "rc4-40", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2", enc_main},
#endif
#ifndef OPENSSL_NO_BF
    {FUNC_TYPE_CIPHER, "bf", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast", enc_main},
#endif
#ifndef OPENSSL_NO_RC5
    {FUNC_TYPE_CIPHER, "rc5", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede3", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede3-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede3-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_DES
    {FUNC_TYPE_CIPHER, "des-ede3-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_IDEA
    {FUNC_TYPE_CIPHER, "idea-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_IDEA
    {FUNC_TYPE_CIPHER, "idea-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_IDEA
    {FUNC_TYPE_CIPHER, "idea-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_IDEA
    {FUNC_TYPE_CIPHER, "idea-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_SEED
    {FUNC_TYPE_CIPHER, "seed-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_SEED
    {FUNC_TYPE_CIPHER, "seed-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_SEED
    {FUNC_TYPE_CIPHER, "seed-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_SEED
    {FUNC_TYPE_CIPHER, "seed-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-64-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_RC2
    {FUNC_TYPE_CIPHER, "rc2-40-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_BF
    {FUNC_TYPE_CIPHER, "bf-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_BF
    {FUNC_TYPE_CIPHER, "bf-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_BF
    {FUNC_TYPE_CIPHER, "bf-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_BF
    {FUNC_TYPE_CIPHER, "bf-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast5-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast5-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast5-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast5-ofb", enc_main},
#endif
#ifndef OPENSSL_NO_CAST
    {FUNC_TYPE_CIPHER, "cast-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_RC5
    {FUNC_TYPE_CIPHER, "rc5-cbc", enc_main},
#endif
#ifndef OPENSSL_NO_RC5
    {FUNC_TYPE_CIPHER, "rc5-ecb", enc_main},
#endif
#ifndef OPENSSL_NO_RC5
    {FUNC_TYPE_CIPHER, "rc5-cfb", enc_main},
#endif
#ifndef OPENSSL_NO_RC5
    {FUNC_TYPE_CIPHER, "rc5-ofb", enc_main},
#endif
    {0, NULL, NULL}
};
