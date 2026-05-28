打印("=== AES-128-CBC ===");
变量 key128 = "0123456789abcdef";
变量 iv16 = "abcdef9876543210";
变量 pt = "Hello CP World!";
变量 ct = aesEncrypt(pt, key128, iv16);
打印("encrypt:", ct);

变量 de = aesDecrypt(ct, key128, iv16);
打印("decrypt:", de);
如果 (de == pt) {
    打印("AES-128 roundtrip OK");
} 否则 {
    打印("AES-128 roundtrip FAILED");
}

打印("=== AES-256-CBC ===");
变量 key256 = "0123456789abcdef0123456789abcdef";
变量 ct2 = aesEncrypt("secret message", key256, iv16);
打印("encrypt256:", ct2);
变量 de2 = aesDecrypt(ct2, key256, iv16);
打印("decrypt256:", de2);
如果 (de2 == "secret message") {
    打印("AES-256 roundtrip OK");
} 否则 {
    打印("AES-256 roundtrip FAILED");
}

打印("=== Random bytes ===");
变量 rb1 = randomBytes(16);
变量 rb2 = randomBytes(16);
打印("random len:", strlen(rb1));
如果 (strlen(rb1) == 16) {
    打印("randomBytes length OK");
} 否则 {
    打印("randomBytes length FAILED");
}
如果 (rb1 != rb2) {
    打印("randomBytes unique OK");
} 否则 {
    打印("randomBytes unique FAILED");
}

打印("=== Dir make ===");
变量 ok = dirMake("tests/_temp_aes/sub1/sub2");
打印("mkdir -p:", ok);
打印("isDir:", fileIsDir("tests/_temp_aes/sub1/sub2"));
dirRemove("tests/_temp_aes/sub1/sub2");
dirRemove("tests/_temp_aes/sub1");
dirRemove("tests/_temp_aes");

打印("ALL_AES_OK");
