the certificates tree is:

CA: means CA:TRUE, can as CA certificate.
R:  means revocate, saved in CRL certificate.
D:  domain(CN): name like www.alice.org/www.alice01.org.
SAN: subjectAltName like CN.

RSA:
alice(1024CA)->alice1(2048R)->alice2(4096CA)->alice3(2048CAR)->alice4(1024)
alice.crl.

DSA:
bob(1024CA)->bob1(2048R)->bob2(4096CA)->bob3(2048CAR)->bob4(1024)
bob.crl

EC:
carl(prime256v1CA)->carl1(prime239v1R)->carl2(prime192v1CA)->carl3(prime192v2CAR)->carl4(prime192v1CA)

Start OCSP server:
          openssl ocsp 	



