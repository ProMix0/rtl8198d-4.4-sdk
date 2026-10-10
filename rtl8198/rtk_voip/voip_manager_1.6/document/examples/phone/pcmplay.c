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


int main(int argv, char** argc)
{
	int fddev = open(DEV_TX0, O_WRONLY);	
	
	if(fddev == -1)
	{
		printf("Can't open PCM device %s\n" , DEV_TX0);
		return fddev;
	}
	
	int fdfile;

	if(argv == 2)
	{
		fdfile = open(argc[1], O_RDONLY);
		printf("Open PCM file - %s\n", argc[1]);
	}
	else
	{
		fdfile = open(VOICE_FILE, O_RDONLY);
		printf("Open default PCM file - %s\n" , VOICE_FILE);
	}

	if(fdfile == -1)
	{
		printf("Can't open PCM file %s\n" , VOICE_FILE);
		close(fddev);
		return -1;
	}

	printf("PCM device and PCM file are opened.\n");


	unsigned char buf[BUF_SIZE];
	//unsigned int loopnum = 0;
	ssize_t count = 0;

	unsigned int playcount = 0;
	while(1)
	{
		while((count = read(fdfile, buf, BUF_SIZE)) > 0 )
		{
			//printf("read count = %d\n", count);
			ssize_t wcount = 0;
			ssize_t n = 0;

			while(count > 0)
			{
				if((n = write(fddev, buf + wcount, count)) < 0)
				{
					printf("write pcm dev error.\n");
				}
				
				wcount += n;
				count -= n;
				//printf("wcount = %d, count = %d\n", wcount, count);
			}
		}

		printf("Replay %d times", playcount);
		playcount++;
		lseek(fdfile, 0, SEEK_SET);
	}

	close(fddev);
	close(fdfile);

	return 0;
}
