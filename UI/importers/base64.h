#ifdef _cplusplus
extern "C" {
#endif

const char *c_base64_encode(unsigned char const *str, unsigned int len);
char *c_base64_decode(const char *str);

#ifdef _cplusplus
}
#endif
