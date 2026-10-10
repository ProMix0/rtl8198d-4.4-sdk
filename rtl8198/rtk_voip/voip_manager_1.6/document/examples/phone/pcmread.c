#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE	2048

#define DEV_TX0 "/dev/voip/pcmtx0"
#define DEV_RX0 "/dev/voip/pcmrx0"

#define DEV_TX1 "/dev/voip/pcmtx1"
#define DEV_RX1 "/dev/voip/pcmrx1"

#define VOICE_FILE "/tmp/voice_record.pcm"

int main(int argc, char** argv)
{

	unsigned char buffer[BUF_SIZE];

	int fdw = open(VOICE_FILE, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR);

	if(fdw == -1)
	{
		printf("Can't open file %s for writing.\n" , VOICE_FILE);
		return -1;
	}

	//int fdpcm = open(DEV_RX0, O_RDWR);
	int fdpcm = open(DEV_RX0, O_RDONLY);

	if(fdpcm == -1)
	{
		printf("Can't open PCM device.\n");
		return -1;
	}

	ssize_t readsize;
	ssize_t wsize;
	ssize_t n;

	while(1)
	{	
		wsize = 0;
		
		if((readsize = read(fdpcm, buffer, BUF_SIZE)) > 0)
		{

			while(readsize > 0)
			{
				n = write(fdw, buffer + wsize, readsize);
				wsize += n;
				readsize -= n;
			}

		}
		else
		{
			//printf("--------------------------------------\n");
		}
	}
	

	return 0;
}
