#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <stdio.h>
#include <strings.h>


#define PORT 53
#define BUFSIZE 2048

int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
	const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const uint8_t *data = (const uint8_t *)data_buf;
	size_t resultIndex = 0;
	size_t x;
	uint32_t n = 0;
	int padCount = dataLength % 3;
	uint8_t n0, n1, n2, n3;

	/* increment over the length of the string, three characters at a time */
	for (x = 0; x < dataLength; x += 3) 
	{
		/* these three 8-bit (ASCII) characters become one 24-bit number */
		n = ((uint32_t)data[x]) << 16; //parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0

		if((x+1) < dataLength)
			n += ((uint32_t)data[x+1]) << 8;//parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0

		if((x+2) < dataLength)
			n += data[x+2];

		/* this 24-bit number gets separated into four 6-bit numbers */
		n0 = (uint8_t)(n >> 18) & 63;
		n1 = (uint8_t)(n >> 12) & 63;
		n2 = (uint8_t)(n >> 6) & 63;
		n3 = (uint8_t)n & 63;

		/*
		 *        * if we have one byte available, then its encoding is spread
		 *               * out over two characters
		 *                      */
		if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
		result[resultIndex++] = base64chars[n0];
		if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
		result[resultIndex++] = base64chars[n1];

		/*
		 *        * if we have only two bytes available, then their encoding is
		 *               * spread out over three chars
		 *                      */
		if((x+1) < dataLength)
		{
			if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
			result[resultIndex++] = base64chars[n2];
		}

		/*
		 *        * if we have all three bytes available, then their encoding is spread
		 *               * out over four characters
		 *                      */
		if((x+2) < dataLength)
		{
			if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
			result[resultIndex++] = base64chars[n3];
		}
	}

	/*
	 *     * create and add padding that is required if we did not have a multiple of 3
	 *         * number of characters available
	 *             */
	if (padCount > 0) 
	{ 
		for (; padCount < 3; padCount++) 
		{ 
			if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
			result[resultIndex++] = '=';
		} 
	}
	if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
	result[resultIndex] = 0;
	return resultIndex;   /* indicate success */
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;      /* our address */
	struct sockaddr_in remaddr;     /* remote address */
	socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
	int recvlen;                    /* # bytes received */
	int fd;                         /* our socket */
	unsigned char buf[BUFSIZE];     /* receive buffer */

	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind failed");
		return 0;
	}
	int try = 0;

	/* now loop, receiving data and printing what we received */
	for (;;) {
		char encoded[256];
		int encoded_len;

		// get request
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		encoded_len = base64encode(buf, recvlen, encoded, 256);

		// do a shitty popen curl request.
		char *cmd  = "curl -s -o /dev/stdout \"https://1.1.1.1/dns-query?ct=application/dns-udpwireformat&dns=";
		char *cmd2 = "\"";
		char *combo_url = malloc((size_t)strlen(cmd) + encoded_len + strlen(cmd2));

		strcat(combo_url, cmd);
		strcat(combo_url, encoded);
		strcat(combo_url, cmd2);

		// open stdout like a bum
		FILE *curl_stdin = popen(combo_url, "r");

		int resplen = 0;
		char respbuf [256];
		while(!feof(curl_stdin)) {
			char byte;
			fread(&byte, 1 , 1, curl_stdin);

			respbuf[resplen] = byte;
			resplen++;
		}
		pclose(curl_stdin);
		free(combo_url);

		// we did it !
		sendto(fd, respbuf, resplen, 0, (struct sockaddr *)&remaddr, addrlen);
	}
	/* never exits, you are stuck forever */
}
