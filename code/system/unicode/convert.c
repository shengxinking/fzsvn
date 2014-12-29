/*
 *	this is a APIs for unicode encoding and decoding.
 *
 */

int utf8_to_unicode(const unsigned char *utf8, size_t size)
{
	int ret;

	if (size < 1 || size > 4)
		return -1;

	switch (size) {
	case 1:
		return utf8[0];
	case 2:
		ret = utf8[0] & 0x1F;
		ret = ret << 6 + utf8[1] & 0x3F;
		return  ret;
	case 3:
		ret = utf8[0] & 0xF;
		ret = ret << 6 + utf8[1] & 0x3F;
		ret = ret << 6 + utf8[2] & 0x3F;
		return ret;
	case 4:
		ret = utf8[0] & 0x7;
		ret = ret << 6 + utf8[1] & 0x3F;
		ret = ret << 6 + utf8[2] & 0x3F;
		ret = ret << 6 + utf8[3] & 0x3F;
		return ret;
	default:
		return -1;

	return -1;
}

int utf16_to_unicode(const unsigned short *utf16, int size)
{
	return -1;
}

int unicode_to_utf8(int usv, unsigned char *utf8, size_t size)
{
	if (size < 4)
		return -1;

	if (usv < 0 || unicode > 0x10FFFF)
		return -1;
	
	if (usv < 0x7F)
		utf8[0] = usv;
	else if (usv < 0x7FF) {
		utf8[0] = 0xC0 + (usv >> 6) & 0x3F;
		utf8[1] = 0x80 + (usv & 0x3F );
	}
	else if (usv < 0xFFFF)
		utf8[0] = usv & 0x
}

int unicode_to_utf16(int unicode, unsigned short *utf16, int size)
{

}

