/* iksemel (XML parser for Jabber)
** Copyright (C) 2000-2003 Gurer Ozen
** This code is free software; you can redistribute it and/or
** modify it under the terms of GNU Lesser General Public License.
*/

#include "iksemel/common.h"
#include "iksemel/iksemel.h"

#define GET_UINT32(n,b,i) {  \
	(n) = ( (unsigned long int) (b)[(i)    ]     )  \
		| ( (unsigned long int) (b)[(i) + 1] <<  8 )  \
		| ( (unsigned long int) (b)[(i) + 2] << 16 )  \
		| ( (unsigned long int) (b)[(i) + 3] << 24 ); \
}

#define PUT_UINT32(n,b,i) {  \
	(b)[(i)    ] = (unsigned char) ( (n)       );  \
	(b)[(i) + 1] = (unsigned char) ( (n) >>  8 );  \
	(b)[(i) + 2] = (unsigned char) ( (n) >> 16 );  \
	(b)[(i) + 3] = (unsigned char) ( (n) >> 24 );  \
}

#define F(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))

#define G(x,y,z) ((y) ^ ((z) & ((x) ^ (y))))

#define H(x,y,z) ((x) ^ (y) ^ (z))

#define I(x,y,z) ((y) ^ ((x) | ~(z)))

#define S(x,n) (((x) << (n)) | (((x) & 0xFFFFFFFF) >> (32 - (n))))

#define P(r,i,f,k,s,t) {  \
	r[i] += f(r[((i)+1)%4],r[((i)+2)%4],r[((i)+3)%4]) + X[k] + t; \
	r[i] = S(r[i],s) + r[((i)+1)%4];  \
}

struct iksmd5_struct {
	unsigned long int total[2];
	unsigned long int state[4];
	unsigned char buffer[64];
	unsigned char blen;
};
#if 0
static const unsigned long int T[] =
	{ 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
#endif
static void iks_md5_compute(iksmd5 *md5);

void ICACHE_FLASH_ATTR
iks_md5_reset(iksmd5 *md5)
{
	os_memset(md5, 0, sizeof(iksmd5));
	md5->state[0] = 0x67452301;
	md5->state[1] = 0xEFCDAB89;
	md5->state[2] = 0x98BADCFE;
	md5->state[3] = 0x10325476;
}

iksmd5 * ICACHE_FLASH_ATTR
iks_md5_new(void)
{
	iksmd5 *md5 = iks_malloc(sizeof(iksmd5));

	if (!md5)
		return NULL;
	iks_md5_reset(md5);
	return md5;
}

void ICACHE_FLASH_ATTR
iks_md5_hash(iksmd5 *md5, const unsigned char *data, size_t slen, int finish)
{
	int i, j;
	int len = slen;

	i = (64 - md5->blen);
	j = (len < i) ? (len) : (i);
	os_memcpy(md5->buffer + md5->blen, data, j);
	md5->blen += j;
	len -= j;
	data += j;
	while (len > 0) {
		iks_md5_compute(md5);
		md5->blen = 0;
		md5->total[0] += 8*64;
		md5->total[1] += (md5->total[0] < 8*64);
		j = (len < 64) ? (len) : (64);
		os_memcpy(md5->buffer, data, j);
		md5->blen = j;
		len -= j;
		data += j;
	}
	if (finish) {
		md5->total[0] += 8*md5->blen;
		md5->total[1] += (md5->total[0] < 8*md5->blen);
		md5->buffer[(md5->blen)++] = 0x80;
		if (md5->blen > 56) {
			while (md5->blen < 64)
				md5->buffer[(md5->blen)++] = 0x00;
			iks_md5_compute(md5);
			md5->blen = 0;
		}
		while (md5->blen < 56)
			md5->buffer[(md5->blen)++] = 0x00;
		PUT_UINT32(md5->total[0], md5->buffer, 56);
		PUT_UINT32(md5->total[1], md5->buffer, 60);
		iks_md5_compute(md5);
	}
}

void ICACHE_FLASH_ATTR
iks_md5_delete(iksmd5 *md5)
{
	iks_free(md5);
}

void ICACHE_FLASH_ATTR
iks_md5_digest(iksmd5 *md5, unsigned char *digest)
{
	PUT_UINT32(md5->state[0], digest,  0);
	PUT_UINT32(md5->state[1], digest,  4);
	PUT_UINT32(md5->state[2], digest,  8);
	PUT_UINT32(md5->state[3], digest, 12);
}

void ICACHE_FLASH_ATTR
iks_md5_print(iksmd5 *md5, char *buf)
{
	int i;
	unsigned char digest[16];

	iks_md5_digest(md5, digest);
	for (i = 0; i < 16; i++) {
		sprintf (buf, "%02x", digest[i]);
		buf += 2;
	}
}

void ICACHE_FLASH_ATTR
iks_md5(const char *data, char *buf)
{
	iksmd5 *md5 = iks_md5_new();

	iks_md5_hash(md5, (const unsigned char*)data, strlen(data), 1);
	iks_md5_print(md5, buf);
	iks_md5_delete(md5);
}

static void ICACHE_FLASH_ATTR
iks_md5_compute(iksmd5 *md5)
{
	unsigned long int X[16], R[4];
	unsigned char RS1[] = { 7, 12 ,17, 22 };
	unsigned char RS2[] = { 5, 9 ,14, 20 };
	unsigned char RS3[] = { 4, 11 ,16, 23 };
	unsigned char RS4[] = { 6, 10 ,15, 21 };
	int i, j, k, p;

	for (i = 0; i < 16; ++i)
		 GET_UINT32(X[i], md5->buffer, i*4);

	for (i = 0; i < 4; ++i)
		R[i] = md5->state[i];
#if 0
	for (i = j = k = 0; i < 16; ++i, j = i%4, k = (k+3)%4)
		P(R, k, F, i, RS1[j], T[i]);

	for (i = j = k = 0, p = 1; i < 16; ++i, j = i%4, k = (k+3)%4, p = (p+5)%16)
		P(R, k, G, p, RS2[j], T[i+16]);

	for (i = j = k = 0, p = 5; i < 16; ++i, j = i%4, k = (k+3)%4, p = (p+3)%16)
		P(R, k, H, p, RS3[j], T[i+32]);

	for (i = j = k = p = 0; i < 16; ++i, j = i%4, k = (k+3)%4, p = (p+7)%16)
		P(R, k, I, p, RS4[j], T[i+48]);
#else
    /* round 1 */
    P(R, 0, F,  0, RS1[0], 0xd76aa478);/* 1 */
    P(R, 3, F,  1, RS1[1], 0xe8c7b756);/* 2 */
    P(R, 2, F,  2, RS1[2], 0x242070db);/* 3 */
    P(R, 1, F,  3, RS1[3], 0xc1bdceee);/* 4 */
    P(R, 0, F,  4, RS1[0], 0xf57c0faf);/* 5 */
    P(R, 3, F,  5, RS1[1], 0x4787c62a);/* 6 */
    P(R, 2, F,  6, RS1[2], 0xa8304613);/* 7 */
    P(R, 1, F,  7, RS1[3], 0xfd469501);/* 8 */
    P(R, 0, F,  8, RS1[0], 0x698098d8);/* 9 */
    P(R, 3, F,  9, RS1[1], 0x8b44f7af);/* 10 */
    P(R, 2, F, 10, RS1[2], 0xffff5bb1);/* 11 */
    P(R, 1, F, 11, RS1[3], 0x895cd7be);/* 12 */
    P(R, 0, F, 12, RS1[0], 0x6b901122);/* 13 */
    P(R, 3, F, 13, RS1[1], 0xfd987193);/* 14 */
    P(R, 2, F, 14, RS1[2], 0xa679438e);/* 15 */
    P(R, 1, F, 15, RS1[3], 0x49b40821);/* 16 */
    
    /* round 2 */
    P(R, 0, G,  1, RS2[0], 0xf61e2562);/* 17 */
    P(R, 3, G,  6, RS2[1], 0xc040b340);/* 18 */
    P(R, 2, G, 11, RS2[2], 0x265e5a51);/* 19 */
    P(R, 1, G,  0, RS2[3], 0xe9b6c7aa);/* 20 */
    P(R, 0, G,  5, RS2[0], 0xd62f105d);/* 21 */
    P(R, 3, G, 10, RS2[1], 0x02441453);/* 22 */
    P(R, 2, G, 15, RS2[2], 0xd8a1e681);/* 23 */
    P(R, 1, G,  4, RS2[3], 0xe7d3fbc8);/* 24 */
    P(R, 0, G,  9, RS2[0], 0x21e1cde6);/* 25 */
    P(R, 3, G, 14, RS2[1], 0xc33707d6);/* 26 */
    P(R, 2, G,  3, RS2[2], 0xf4d50d87);/* 27 */
    P(R, 1, G,  8, RS2[3], 0x455a14ed);/* 28 */
    P(R, 0, G, 13, RS2[0], 0xa9e3e905);/* 29 */
    P(R, 3, G,  2, RS2[1], 0xfcefa3f8);/* 30 */
    P(R, 2, G,  7, RS2[2], 0x676f02d9);/* 31 */
    P(R, 1, G, 12, RS2[3], 0x8d2a4c8a);/* 32 */
    
    /* round 3 */
    P(R, 0, H,  5, RS3[0], 0xfffa3942);/* 33 */
    P(R, 3, H,  8, RS3[1], 0x8771f681);/* 34 */
    P(R, 2, H, 11, RS3[2], 0x6d9d6122);/* 35 */
    P(R, 1, H, 14, RS3[3], 0xfde5380c);/* 36 */
    P(R, 0, H,  1, RS3[0], 0xa4beea44);/* 37 */
    P(R, 3, H,  4, RS3[1], 0x4bdecfa9);/* 38 */
    P(R, 2, H,  7, RS3[2], 0xf6bb4b60);/* 39 */
    P(R, 1, H, 10, RS3[3], 0xbebfbc70);/* 40 */
    P(R, 0, H, 13, RS3[0], 0x289b7ec6);/* 41 */
    P(R, 3, H,  0, RS3[1], 0xeaa127fa);/* 42 */
    P(R, 2, H,  3, RS3[2], 0xd4ef3085);/* 43 */
    P(R, 1, H,  6, RS3[3], 0x04881d05);/* 44 */
    P(R, 0, H,  9, RS3[0], 0xd9d4d039);/* 45 */
    P(R, 3, H, 12, RS3[1], 0xe6db99e5);/* 46 */
    P(R, 2, H, 15, RS3[2], 0x1fa27cf8);/* 47 */
    P(R, 1, H,  2, RS3[3], 0xc4ac5665);/* 48 */
    
    /* round 4 */
    P(R, 0, I,  0, RS4[0], 0xf4292244);/* 49 */
    P(R, 3, I,  7, RS4[1], 0x432aff97);/* 50 */
    P(R, 2, I, 14, RS4[2], 0xab9423a7);/* 51 */
    P(R, 1, I,  5, RS4[3], 0xfc93a039);/* 52 */
    P(R, 0, I, 12, RS4[0], 0x655b59c3);/* 53 */
    P(R, 3, I,  3, RS4[1], 0x8f0ccc92);/* 54 */
    P(R, 2, I, 10, RS4[2], 0xffeff47d);/* 55 */
    P(R, 1, I,  1, RS4[3], 0x85845dd1);/* 56 */
    P(R, 0, I,  8, RS4[0], 0x6fa87e4f);/* 57 */
    P(R, 3, I, 15, RS4[1], 0xfe2ce6e0);/* 58 */
    P(R, 2, I,  6, RS4[2], 0xa3014314);/* 59 */
    P(R, 1, I, 13, RS4[3], 0x4e0811a1);/* 60 */
    P(R, 0, I,  4, RS4[0], 0xf7537e82);/* 61 */
    P(R, 3, I, 11, RS4[1], 0xbd3af235);/* 62 */
    P(R, 2, I,  2, RS4[2], 0x2ad7d2bb);/* 63 */
    P(R, 1, I,  9, RS4[3], 0xeb86d391);/* 64 */
#endif
	for (i = 0; i < 4; ++i)
		md5->state[i] += R[i];
}
