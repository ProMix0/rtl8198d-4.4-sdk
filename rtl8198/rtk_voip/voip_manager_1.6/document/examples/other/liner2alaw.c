#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>


#define SEG_SHIFT       4
#define QUANT_MASK      0xf
#define BUF_SIZE        2048
static  short  seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

char Linear2Ulaw(int linear_sample) ;
char linear2alaw(int pcm_val);

int Pcm2alaw(const char *, const char*);
int GetFileSize( const char * szFileName );
void AddPcmalawHdr(FILE* fp , int nFileSize);

static int search(int val, short *table, int size)
{
	int i;

	for (i = 0; i < size; i++)
	{
		if (val <= *table++)
			return (i);
    }
	return (size);
}

 
int main(int argc, char *argv[])
{
	rtk_GetVoIPFeature();
		
	/* convert PCM 16bit liner to G711 alaw file for IVR usage */
    Pcm2alaw("test.pcm","test711a.wav");
	return 0;
}


 
char linear2alaw(int pcm_val)		  /* 2's complement (16-bit range) */
{
	int mask;
	int seg;
	unsigned char aval;
 
	if (pcm_val >= 0)
	{
		mask = 0xD5;		 /* sign (7th) bit = 1 */
	}
	else
	{
		mask = 0x55;		 /* sign bit = 0 */
		pcm_val = ~pcm_val;
	}
 
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);
 
	/* Combine the sign, segment, and quantization bits. */
 
	if (seg >= 8)		 /* out of range, return maximum value. */
		return (0x7F ^ mask);
	else
	{
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}


 
int Pcm2alaw(const char* szInFile , const char* szOutFile)
{
    FILE *fin = NULL , *fout = NULL;
    int nFileSize = 0;
    int nRet = 0;
    short *psBuff = NULL;
    unsigned char *pucBuff = NULL;
    int nSample = 0;
    int i = 0; 
   
    nFileSize = GetFileSize(szInFile);
    if(nFileSize <2)
        goto err_out;
 
    if( !(fin = fopen(szInFile,"rb")))
        goto err_out;
 
    if( !(fout = fopen(szOutFile,"wb")))
        goto err_out;
 
    psBuff  = (short*)malloc(nFileSize);
    pucBuff = (unsigned char*)malloc(nFileSize/sizeof(short));
   
    if(!psBuff)
        goto err_out;
    if(!pucBuff)
        goto err_out;
 
    fread(psBuff,nFileSize,1,fin);
 
    nSample = nFileSize / sizeof(short);
 
    for(i=0;i<nSample;i++) {
       pucBuff[i] = linear2alaw(psBuff[i]);
    }
   
    AddPcmalawHdr(fout, nSample * sizeof(unsigned char));
    fwrite(pucBuff,sizeof(unsigned char),nSample,fout);
 
    nRet = 1;
 
err_out:
    if(fin) fclose(fin);
    if(fout) fclose(fout);
    if(psBuff) free(psBuff);
    if(pucBuff) free(pucBuff);
    return nRet;
}
 
int GetFileSize( const char * szFileName )
{
	struct stat fileStat;
	int err = stat( szFileName, &fileStat );

    if (0 != err) return 0;
	return fileStat.st_size;
}
 
void AddPcmalawHdr(FILE* fp , int nFileSize)
{
    int   nChunkSize = 0;
    int   nSubchunk1Size = 18;
    short sAudioFormat = 6; //alaw 
    short sChannel = 1;
    short sBitsPerSampl = 8;
    short sReserve = 0;
    int   nSampleRate = 8000;
    int   nByteRate = nSampleRate * sChannel * sBitsPerSampl / 8;
    short sBlockAlign = sChannel * sBitsPerSampl / 8;
    int   nSubchunk2Size = 0;   
   
    nSubchunk2Size = nFileSize;
 
    fprintf(fp,"RIFF");                        //ChunkID : 4
    nChunkSize = nFileSize + 0x24;           
    fwrite(&nChunkSize,sizeof(int),1,fp);      //ChunkSize : 4
    fprintf(fp,"WAVE");         //Format : 4
    fprintf(fp,"fmt ");                        //Subchunk1ID : 4
    fwrite(&nSubchunk1Size,sizeof(int),1,fp);  //Subchunk1Size : 4
    fwrite(&sAudioFormat,sizeof(short),1,fp);  //AudioFormat : 2
    fwrite(&sChannel,sizeof(short),1,fp);      //NumChannels : 2
    fwrite(&nSampleRate,sizeof(int),1,fp);     //SampleRate : 4
    fwrite(&nByteRate,sizeof(int),1,fp);        //ByteRate : 4
    fwrite(&sBlockAlign,sizeof(short),1,fp);   //BlockAlign : 2
    fwrite(&sBitsPerSampl,sizeof(short),1,fp);  //BitsPerSample : 2
    fwrite(&sReserve,sizeof(short),1,fp);       // Reserved : 2
    fprintf(fp,"data");                        //Subchunk2ID : 4
    fwrite(&nSubchunk2Size,sizeof(int),1,fp);  //Subchunk2Size : 4 
}


