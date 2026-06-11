# Test-only TLS fixtures

Self-signed certificate, key, and DH parameters used solely by
`test_tls_context.cpp` to smoke-test `make_tls_context`. They are
trusted by nothing, deployed nowhere, and carry no secret — do not reuse
them outside the test. Regenerate at will:

```sh
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem \
  -days 3650 -nodes -subj "/CN=beatled-test"
openssl dhparam -out dh_param.pem 2048
```
