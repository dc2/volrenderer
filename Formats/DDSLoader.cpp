// (c) by Stefan Roettger, licensed under GPL 2+
// see https://code.google.com/p/vvv/

#include "DDSLoader.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#define DDS_MAXSTR (256)

#define DDS_BLOCKSIZE (1<<20)
#define DDS_INTERLEAVE (1<<24)

#define DDS_RL (7)

#define DDS_ISINTEL (*((uint8_t *)(&DDS_INTEL)+1)==0)

#define ERRORMSG() errormsg(__FILE__,__LINE__)




//#include "codebase.h" // universal code base

uint8_t *readPVMvolume(const char *filename,
                             unsigned int *width,unsigned int *height,unsigned int *depth,unsigned int *components=NULL,
                             float *scalex=NULL,float *scaley=NULL,float *scalez=NULL,
                             uint8_t **description=NULL,
                             uint8_t **courtesy=NULL,
                             uint8_t **parameter=NULL,
                             uint8_t **comment=NULL);


inline void errormsg(const char *file,int line)
{
    fprintf(stderr,"fatal error in <%s> at line %d!\n",file,line);
    exit(EXIT_FAILURE);
}


char DDS_ID[]="DDS v3d\n";
char DDS_ID2[]="DDS v3e\n";

uint8_t *DDS_cache;
unsigned int DDS_cachepos, DDS_cachesize;

unsigned int DDS_buffer;
unsigned int DDS_bufsize;

unsigned short int DDS_INTEL=1;

// helper functions for DDS:

inline unsigned int DDS_shiftl(const unsigned int value,const unsigned int bits)
{return((bits>=32)?0:value<<bits);}

inline unsigned int DDS_shiftr(const unsigned int value,const unsigned int bits)
{return((bits>=32)?0:value>>bits);}

inline void DDS_swapuint(unsigned int *x)
{
    unsigned int tmp=*x;
    
    *x=((tmp&0xff)<<24)|
            ((tmp&0xff00)<<8)|
            ((tmp&0xff0000)>>8)|
            ((tmp&0xff000000)>>24);
}

void DDS_initbuffer()
{
    DDS_buffer=0;
    DDS_bufsize=0;
}

inline void DDS_clearbits()
{
    DDS_cache=NULL;
    DDS_cachepos=0;
    DDS_cachesize=0;
}

inline void DDS_loadbits(uint8_t *data,unsigned int size)
{
    DDS_cache=data;
    DDS_cachesize=size;
    
    if ((DDS_cache=(uint8_t *)realloc(DDS_cache,DDS_cachesize+4))==NULL) ERRORMSG();
    *((unsigned int *)&DDS_cache[DDS_cachesize])=0;
    
    DDS_cachesize=4*((DDS_cachesize+3)/4);
    if ((DDS_cache=(uint8_t *)realloc(DDS_cache,DDS_cachesize))==NULL) ERRORMSG();
}

inline unsigned int DDS_readbits(unsigned int bits)
{
    unsigned int value;
    
    if (bits<DDS_bufsize)
    {
        DDS_bufsize-=bits;
        value=DDS_shiftr(DDS_buffer,DDS_bufsize);
    }
    else
    {
        value=DDS_shiftl(DDS_buffer,bits-DDS_bufsize);
        
        if (DDS_cachepos>=DDS_cachesize) DDS_buffer=0;
        else
        {
            DDS_buffer=*((unsigned int *)&DDS_cache[DDS_cachepos]);
            if (DDS_ISINTEL) DDS_swapuint(&DDS_buffer);
            DDS_cachepos+=4;
        }
        
        DDS_bufsize+=32-bits;
        value|=DDS_shiftr(DDS_buffer,DDS_bufsize);
    }
    
    DDS_buffer&=DDS_shiftl(1,DDS_bufsize)-1;
    
    return(value);
}

inline int DDS_code(int bits)
{return(bits>1?bits-1:bits);}

inline int DDS_decode(int bits)
{return(bits>=1?bits+1:bits);}

// deinterleave a byte stream
void DDS_deinterleave(uint8_t *data,unsigned int bytes,unsigned int skip,unsigned int block=0,bool restore=false)
{
    unsigned int i,j,k;
    
    uint8_t *data2,*ptr;
    
    if (skip<=1) return;
    
    if (block==0)
    {
        if ((data2=(uint8_t *)malloc(bytes))==NULL) ERRORMSG();
        
        if (!restore)
            for (ptr=data2,i=0; i<skip; i++)
                for (j=i; j<bytes; j+=skip) *ptr++=data[j];
        else
            for (ptr=data,i=0; i<skip; i++)
                for (j=i; j<bytes; j+=skip) data2[j]=*ptr++;
        
        memcpy(data,data2,bytes);
    }
    else
    {
        if ((data2=(uint8_t *)malloc((bytes<skip*block)?bytes:skip*block))==NULL) ERRORMSG();
        
        if (!restore)
        {
            for (k=0; k<bytes/skip/block; k++)
            {
                for (ptr=data2,i=0; i<skip; i++)
                    for (j=i; j<skip*block; j+=skip) *ptr++=data[k*skip*block+j];
                
                memcpy(data+k*skip*block,data2,skip*block);
            }
            
            for (ptr=data2,i=0; i<skip; i++)
                for (j=i; j<bytes-k*skip*block; j+=skip) *ptr++=data[k*skip*block+j];
            
            memcpy(data+k*skip*block,data2,bytes-k*skip*block);
        }
        else
        {
            for (k=0; k<bytes/skip/block; k++)
            {
                for (ptr=data+k*skip*block,i=0; i<skip; i++)
                    for (j=i; j<skip*block; j+=skip) data2[j]=*ptr++;
                
                memcpy(data+k*skip*block,data2,skip*block);
            }
            
            for (ptr=data+k*skip*block,i=0; i<skip; i++)
                for (j=i; j<bytes-k*skip*block; j+=skip) data2[j]=*ptr++;
            
            memcpy(data+k*skip*block,data2,bytes-k*skip*block);
        }
    }
    
    free(data2);
}

// interleave a byte stream
void DDS_interleave(uint8_t *data,unsigned int bytes,unsigned int skip,unsigned int block=0)
{DDS_deinterleave(data,bytes,skip,block,true);}

// decode a Differential Data Stream
void DDS_decode(uint8_t *chunk,unsigned int size,
                uint8_t **data,unsigned int *bytes,
                unsigned int block=0)
{
    unsigned int skip,strip;
    
    uint8_t *ptr1,*ptr2;
    
    unsigned int cnt,cnt1,cnt2;
    int bits,act;
    
    DDS_initbuffer();
    
    DDS_clearbits();
    DDS_loadbits(chunk,size);
    
    skip=DDS_readbits(2)+1;
    strip=DDS_readbits(16)+1;
    
    ptr1=ptr2=NULL;
    cnt=act=0;
    
    while ((cnt1=DDS_readbits(DDS_RL))!=0)
    {
        bits=DDS_decode(DDS_readbits(3));
        
        for (cnt2=0; cnt2<cnt1; cnt2++)
        {
            if (strip==1 || cnt<=strip) act+=DDS_readbits(bits)-(1<<bits)/2;
            else act+=*(ptr2-strip)-*(ptr2-strip-1)+DDS_readbits(bits)-(1<<bits)/2;
            
            while (act<0) act+=256;
            while (act>255) act-=256;
            
            if ((cnt&(DDS_BLOCKSIZE-1))==0) {
                if (ptr1==NULL)
                {
                    if ((ptr1=(uint8_t *)malloc(DDS_BLOCKSIZE))==NULL) ERRORMSG();
                    ptr2=ptr1;
                }
                else
                {
                    if ((ptr1=(uint8_t *)realloc(ptr1,cnt+DDS_BLOCKSIZE))==NULL) ERRORMSG();
                    ptr2=&ptr1[cnt];
                }
            }
            
            *ptr2++=act;
            cnt++;
        }
    }
    
    if (ptr1!=NULL)
        if ((ptr1=(uint8_t *)realloc(ptr1,cnt))==NULL) ERRORMSG();
    
    DDS_interleave(ptr1,cnt,skip,block);
    
    *data=ptr1;
    *bytes=cnt;
}

// read from a RAW file
uint8_t *readRAWfiled(FILE *file,unsigned int *bytes)
{
    uint8_t *data;
    unsigned int cnt,blkcnt;
    
    data=NULL;
    cnt=0;
    
    do
    {
        if (data==NULL)
        {if ((data=(uint8_t *)malloc(DDS_BLOCKSIZE))==NULL) ERRORMSG();}
        else
            if ((data=(uint8_t *)realloc(data,cnt+DDS_BLOCKSIZE))==NULL) ERRORMSG();
        
        blkcnt=fread(&data[cnt],1,DDS_BLOCKSIZE,file);
        cnt+=blkcnt;
    }
    while (blkcnt==DDS_BLOCKSIZE);
    
    if (cnt==0)
    {
        free(data);
        return(NULL);
    }
    
    if ((data=(uint8_t *)realloc(data,cnt))==NULL) ERRORMSG();
    
    *bytes=cnt;
    
    return(data);
}

// read a RAW file
uint8_t *readRAWfile(const char *filename, unsigned int *bytes)
{
    FILE *file;
    
    uint8_t *data;
    
    if ((file=fopen(filename,"rb"))==NULL) return(NULL);
    
    data=readRAWfiled(file,bytes);
    
    fclose(file);
    
    return(data);
}

// read a Differential Data Stream
uint8_t *readDDSfile(const char *filename, unsigned int *bytes)
{
    int version=1;
    
    FILE *file;
    
    int cnt;
    
    uint8_t *chunk,*data;
    unsigned int size;
    
    if ((file=fopen(filename,"rb"))==NULL) {
        return(NULL);
    }
    
    for (cnt=0; DDS_ID[cnt]!='\0'; cnt++) {
        if (fgetc(file)!=DDS_ID[cnt])
        {
            fclose(file);
            version=0;
            break;
        }
    }
    
    if (version==0)
    {
        if ((file=fopen(filename,"rb"))==NULL) return(NULL);
        
        for (cnt=0; DDS_ID2[cnt]!='\0'; cnt++)
            if (fgetc(file)!=DDS_ID2[cnt])
            {
                fclose(file);
                return(NULL);
            }
        
        version=2;
    }
    
    if ((chunk=readRAWfiled(file,&size))==NULL) ERRORMSG();
    
    fclose(file);
    
    DDS_decode(chunk,size,&data,bytes,version==1?0:DDS_INTERLEAVE);
    
    free(chunk);
    
    return(data);
}

void swapshort(uint8_t *ptr,unsigned int size)
{
    unsigned int i;
    
    uint8_t lo,hi;
    
    for (i=0; i<size; i++)
    {
        lo=ptr[0];
        hi=ptr[1];
        *ptr++=hi;
        *ptr++=lo;
    }
}

// read a possibly compressed PNM image
uint8_t *readPNMimage(const char *filename,unsigned int *width,unsigned int *height,unsigned int *components)
{
    const int maxstr=100;
    
    char str[maxstr];
    
    uint8_t *data,*ptr1,*ptr2;
    unsigned int bytes;
    
    int pnmtype,maxval;
    uint8_t *image;
    
    if ((data=readDDSfile(filename,&bytes))==NULL)
        if ((data=readRAWfile(filename,&bytes))==NULL) return(NULL);
    
    if (bytes<4) return(NULL);
    
    memcpy(str,data,3);
    str[3]='\0';
    
    if (sscanf(str,"P%1d\n",&pnmtype)!=1) return(NULL);
    
    ptr1=data+3;
    while (*ptr1=='\n' || *ptr1=='#')
    {
        while (*ptr1=='\n')
            if (++ptr1>=data+bytes) ERRORMSG();
        while (*ptr1=='#')
            if (++ptr1>=data+bytes) ERRORMSG();
            else
                while (*ptr1!='\n')
                    if (++ptr1>=data+bytes) ERRORMSG();
    }
    
    ptr2=ptr1;
    while (*ptr2!='\n' && *ptr2!=' ')
        if (++ptr2>=data+bytes) ERRORMSG();
    if (++ptr2>=data+bytes) ERRORMSG();
    while (*ptr2!='\n' && *ptr2!=' ')
        if (++ptr2>=data+bytes) ERRORMSG();
    if (++ptr2>=data+bytes) ERRORMSG();
    while (*ptr2!='\n' && *ptr2!=' ')
        if (++ptr2>=data+bytes) ERRORMSG();
    if (++ptr2>=data+bytes) ERRORMSG();
    
    if (ptr2-ptr1>=maxstr) ERRORMSG();
    memcpy(str,ptr1,ptr2-ptr1);
    str[ptr2-ptr1]='\0';
    
    if (sscanf(str,"%d %d\n%d\n",width,height,&maxval)!=3) ERRORMSG();
    
    if (*width<1 || *height<1) ERRORMSG();
    
    if (pnmtype==5 && maxval==255) *components=1;
    else if (pnmtype==5 && (maxval==32767 || maxval==65535)) *components=2;
    else if (pnmtype==6 && maxval==255) *components=3;
    else ERRORMSG();
    
    if ((image=(uint8_t *)malloc((*width)*(*height)*(*components)))==NULL) ERRORMSG();
    if (data+bytes!=ptr2+(*width)*(*height)*(*components)) ERRORMSG();
    
    memcpy(image,ptr2,(*width)*(*height)*(*components));
    free(data);
    
    return(image);
}

// read a compressed PVM volume
uint8_t *readPVMvolume(const char *filename,
                       unsigned int *width,unsigned int *height,unsigned int *depth,unsigned int *components,
                       float *scalex,float *scaley,float *scalez,
                       uint8_t **description,
                       uint8_t **courtesy,
                       uint8_t **parameter,
                       uint8_t **comment)
{
    uint8_t *data,*ptr;
    unsigned int bytes,numc;
    
    int version=1;
    
    uint8_t *volume;
    
    float sx=1.0f,sy=1.0f,sz=1.0f;
    
    unsigned int len1=0,len2=0,len3=0,len4=0;
    
    if ((data=readDDSfile(filename,&bytes))==NULL)
        if ((data=readRAWfile(filename,&bytes))==NULL) return(NULL);
    
    if (bytes<5) return(NULL);
    
    if ((data=(uint8_t *)realloc(data,bytes+1))==NULL) ERRORMSG();
    data[bytes]='\0';
    
    if (strncmp((char *)data,"PVM\n",4)!=0)
    {
        if (strncmp((char *)data,"PVM2\n",5)==0) version=2;
        else if (strncmp((char *)data,"PVM3\n",5)==0) version=3;
        else return(NULL);
        
        ptr=&data[5];
        if (sscanf((char *)ptr,"%d %d %d\n%g %g %g\n",width,height,depth,&sx,&sy,&sz)!=6) ERRORMSG();
        if (*width<1 || *height<1 || *depth<1 || sx<=0.0f || sy<=0.0f || sz<=0.0f) ERRORMSG();
        ptr=(uint8_t *)strchr((char *)ptr,'\n')+1;
    }
    else
    {
        ptr=&data[4];
        while (*ptr=='#')
            while (*ptr++!='\n');
        
        if (sscanf((char *)ptr,"%d %d %d\n",width,height,depth)!=3) ERRORMSG();
        if (*width<1 || *height<1 || *depth<1) ERRORMSG();
    }
    
    if (scalex!=NULL && scaley!=NULL && scalez!=NULL)
    {
        *scalex=sx;
        *scaley=sy;
        *scalez=sz;
    }
    
    ptr=(uint8_t *)strchr((char *)ptr,'\n')+1;
    if (sscanf((char *)ptr,"%d\n",&numc)!=1) ERRORMSG();
    if (numc<1) ERRORMSG();
    
    if (components!=NULL) *components=numc;
    else if (numc!=1) ERRORMSG();
    
    ptr=(uint8_t *)strchr((char *)ptr,'\n')+1;
    if (version==3) len1=strlen((char *)(ptr+(*width)*(*height)*(*depth)*numc))+1;
    if (version==3) len2=strlen((char *)(ptr+(*width)*(*height)*(*depth)*numc+len1))+1;
    if (version==3) len3=strlen((char *)(ptr+(*width)*(*height)*(*depth)*numc+len1+len2))+1;
    if (version==3) len4=strlen((char *)(ptr+(*width)*(*height)*(*depth)*numc+len1+len2+len3))+1;
    if ((volume=(uint8_t *)malloc((*width)*(*height)*(*depth)*numc+len1+len2+len3+len4))==NULL) ERRORMSG();
    if (data+bytes!=ptr+(*width)*(*height)*(*depth)*numc+len1+len2+len3+len4) ERRORMSG();
    
    memcpy(volume,ptr,(*width)*(*height)*(*depth)*numc+len1+len2+len3+len4);
    free(data);
    
    if (description!=NULL) {
        if (len1>1) *description=volume+(*width)*(*height)*(*depth)*numc;
    } else {
        //*description=NULL;
    }
    
    if (courtesy!=NULL) {
        if (len2>1) *courtesy=volume+(*width)*(*height)*(*depth)*numc+len1;
    } else {
        //*courtesy=NULL;
    }
    
    if (parameter!=NULL) {
        if (len3>1) *parameter=volume+(*width)*(*height)*(*depth)*numc+len1+len2;
    } else {
        //*parameter=NULL;
    }
    
    if (comment!=NULL) {
        if (len4>1) *comment=volume+(*width)*(*height)*(*depth)*numc+len1+len2+len3;
    } else {
        //*comment=NULL;
    }
    
    return(volume);
}

uint8_t *DDSLoader::loadFile(const QString &filename)
{
    unsigned int components;
    uint8_t *raw = readPVMvolume(filename.toStdString().c_str(), &width, &height, &depth, &components);
    
    bitDepth = components*8;
    
    int voxelCount = width*height*depth;
    
    uint8_t *dst = new uint8_t[voxelCount*components];
    
    normalizeData(voxelCount, ByteOrder::BO_BIG_ENDIAN, components, raw, dst);
    
    free(raw);
    
    return dst;
}
