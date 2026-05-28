// test_hmac_md5.cp - HMAC-MD5 RFC 2202 test vectors
// Test vector 2: key="Jefe", data="what do ya want for nothing?"
打印("A")
a = hmacMd5("Jefe", "what do ya want for nothing?")
打印(a)
打印("B")
// Test vector 1 (binary key approximated with string)
b = hmacMd5("AAAAAAAAAAAAAAAA", "Hi There")
打印(b)
打印("C")
// MD5 still works
c = md5("")
打印(c)
d = md5("hello")
打印(d)
打印("HMAC_PASS")
