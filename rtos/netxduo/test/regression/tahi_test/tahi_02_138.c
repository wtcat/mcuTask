#include "nx_api.h"
#if defined(NX_TAHI_ENABLE) && defined(FEATURE_NX_IPV6)  

#include "netx_tahi.h"

static char pkt1[] = {
0x33, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 
0x1d, 0xd6, 0x40, 0x00, 0x00, 0x2d, 0x00, 0x00, 
0x27, 0x10, 0x00, 0x00, 0x03, 0xe8, 0x03, 0x04, 
0x40, 0xc0, 0x00, 0x27, 0x8d, 0x00, 0x00, 0x09, 
0x3a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#if 0
static char pkt2[] = {
0x33, 0x33, 0xff, 0x33, 0x44, 0x56, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x18, 0x3a, 0xff, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x33, 0x44, 0x56, 0x87, 0x00, 
0x88, 0x85, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56 };

static char pkt3[] = {
0x33, 0x33, 0xff, 0x33, 0x44, 0x56, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x18, 0x3a, 0xff, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x33, 0x44, 0x56, 0x87, 0x00, 
0x88, 0x85, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56 };
#endif

static char pkt4[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt5[] = {
0x33, 0x33, 0xff, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x00, 0xa0, 0xa0, 0x87, 0x00, 
0x24, 0xa9, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };

static char pkt6[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x88, 0x00, 
0x07, 0x26, 0xe0, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x02, 0x01, 
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0 };

static char pkt7[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt8[] = {
0x33, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0xa1, 0xa1, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 
0x1c, 0xd5, 0x40, 0x00, 0x00, 0x2d, 0x00, 0x00, 
0x27, 0x10, 0x00, 0x00, 0x03, 0xe8, 0x03, 0x04, 
0x40, 0xc0, 0x00, 0x27, 0x8d, 0x00, 0x00, 0x09, 
0x3a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static char pkt9[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt10[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt11[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt12[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt13[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt14[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt15[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt16[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt17[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt18[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt19[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt20[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x81, 0x00, 
0xeb, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt21[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x87, 0x00, 
0x23, 0x2d, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };

#if 0
static char pkt22[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x87, 0x00, 
0x23, 0x2d, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };

static char pkt23[] = {
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x87, 0x00, 
0x23, 0x2d, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };
#endif

static char pkt24[] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x10, 0x3a, 0x40, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x80, 0x00, 
0xec, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char pkt25[] = {
0x33, 0x33, 0xff, 0x00, 0xa1, 0xa1, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x00, 0xa1, 0xa1, 0x87, 0x00, 
0x22, 0xa7, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };

#if 0
static char pkt26[] = {
0x33, 0x33, 0xff, 0x00, 0xa1, 0xa1, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x00, 0xa1, 0xa1, 0x87, 0x00, 
0x22, 0xa7, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };

static char pkt27[] = {
0x33, 0x33, 0xff, 0x00, 0xa1, 0xa1, 0x00, 0x11, 
0x22, 0x33, 0x44, 0x56, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, 
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0x00, 0xa1, 0xa1, 0x87, 0x00, 
0x22, 0xa7, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0x01, 0x01, 
0x00, 0x11, 0x22, 0x33, 0x44, 0x56 };
#endif

static char pkt28[] = {
0x33, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0xa1, 0xa1, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 
0x96, 0x92, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x75, 0x30, 0x00, 0x00, 0x03, 0xe8, 0x03, 0x04, 
0x40, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static char pkt29[] = {
0x33, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, 
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0xff, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 
0x97, 0x93, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x75, 0x30, 0x00, 0x00, 0x03, 0xe8, 0x03, 0x04, 
0x40, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

TAHI_TEST_SEQ tahi_02_138[] = {
    {TITLE, "02-138", 6, 0},

    {INJECT, &pkt1[0], sizeof(pkt1), 0},
    {WAIT, NX_NULL, 0, 5},

    {INJECT, &pkt4[0], sizeof(pkt4), 0},
    {CHECK, &pkt5[0], sizeof(pkt5), 2},
    {INJECT, &pkt6[0], sizeof(pkt6), 0},
    {CHECK, &pkt7[0], sizeof(pkt7), 2},
    {INJECT, &pkt8[0], sizeof(pkt8), 0},
    {INJECT, &pkt9[0], sizeof(pkt9), 0},
    {CHECK, &pkt10[0], sizeof(pkt10), 2},
    {N_CHECK, (char *)NS, 0, 3},

    {INJECT, &pkt11[0], sizeof(pkt11), 0},
    {CHECK, &pkt12[0], sizeof(pkt12), 2},
    {N_CHECK, (char *)NS, 0, 3},

    {INJECT, &pkt13[0], sizeof(pkt13), 0},
    {CHECK, &pkt14[0], sizeof(pkt14), 2}, 
    {N_CHECK, (char *)NS, 0, 3},

    {INJECT, &pkt15[0], sizeof(pkt15), 0},
    {CHECK, &pkt16[0], sizeof(pkt16), 2}, 
    {N_CHECK, (char *)NS, 0, 3},

    {INJECT, &pkt17[0], sizeof(pkt17), 0},
    {CHECK, &pkt18[0], sizeof(pkt18), 2}, 
    {WAIT, NX_NULL, 0, 3},

    {INJECT, &pkt19[0], sizeof(pkt19), 0},
    {CHECK, &pkt20[0], sizeof(pkt20), 2},
    {CHECK, &pkt21[0], sizeof(pkt21), 3},
    {WAIT, NX_NULL, 0, 4},

    {INJECT, &pkt24[0], sizeof(pkt24), 0},
    {CHECK, &pkt25[0], sizeof(pkt25), 2},
    {WAIT, NX_NULL, 0, 4},

    {INJECT, &pkt28[0], sizeof(pkt28), 0},
    {INJECT, &pkt29[0], sizeof(pkt29), 0},

    {CLEANUP, NX_NULL, 0, 0},
    {DUMP, NX_NULL, 0, 0}
};

int tahi_02_138_size = sizeof(tahi_02_138) / sizeof(TAHI_TEST_SEQ);

#endif /* NX_TAHI_ENABLE */