#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PIPE1 "RESP_PIPE_65781"
#define PIPE2 "REQ_PIPE_65781"

int verificareFisierSF(char* data,off_t size,unsigned int detaliiSectiune[2],int nrSectiune){
    if(data[size-1]!='g'){
        return -1;
    }
    unsigned int sizeHeader=256*(data[size-2]& 0xff)+(data[size-3]&0xff);
    unsigned int newOffset=size-sizeHeader;

    unsigned int version=256*256*256*(data[newOffset+3]&0xff)+256*256*(data[newOffset+2]&0xff)+
                        256*(data[newOffset+1]&0xff)+(data[newOffset]&0xff);
    if(version<49 || version>113){
        return -1;
    }

    unsigned int nrSections=(data[newOffset+4]&0xff);
    if(nrSections<10 || nrSections>13){
        return -1;
    }

    unsigned int locatie=newOffset+5;
    for(int i=0; i<nrSections; i++){
        locatie+=18;
        unsigned int type=256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        if(!(type==69||type==30||type==75||type==23||type==33||type==98||type==58)){
            return -1;
        }
        locatie+=4;
        unsigned offset =256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        locatie+=4;
        unsigned int sizeS=256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        locatie+=4;
        if((i+1)==nrSectiune){
            detaliiSectiune[0]=offset;
            detaliiSectiune[1]=sizeS;
            detaliiSectiune[2]=nrSections;
        }

    }
    return 0;
}

int verificareFisierSFLogical(char* data,off_t size,unsigned int logicalOffset,unsigned int detaliiSectiune[3]){
    if(data[size-1]!='g'){
        return -1;
    }
    unsigned int sizeHeader=256*(data[size-2]& 0xff)+(data[size-3]&0xff);
    unsigned int newOffset=size-sizeHeader;

    unsigned int version=256*256*256*(data[newOffset+3]&0xff)+256*256*(data[newOffset+2]&0xff)+
                        256*(data[newOffset+1]&0xff)+(data[newOffset]&0xff);
    if(version<49 || version>113){
        return -1;
    }

    unsigned int nrSections=(data[newOffset+4]&0xff);
    if(nrSections<10 || nrSections>13){
        return -1;
    }

    unsigned int offsetCalculat=0, ok=0;
    unsigned int locatie=newOffset+5;
    for(int i=0; i<nrSections; i++){
        locatie+=18;
        unsigned int type=256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        if(!(type==69||type==30||type==75||type==23||type==33||type==98||type==58)){
            return -1;
        }
        locatie+=4;
        unsigned offset =256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        locatie+=4;
        unsigned int sizeS=256*256*256*(data[locatie+3]&0xff)+256*256*(data[locatie+2]&0xff)+
                        256*(data[locatie+1]&0xff)+(data[locatie]&0xff);
        locatie+=4;
        if(logicalOffset<(offsetCalculat+sizeS) && ok==0){
            ok=1;
            detaliiSectiune[0]=offset;
            detaliiSectiune[1]=sizeS;
            detaliiSectiune[2]=offsetCalculat;
        }
        int coef=0;
        while(sizeS>coef){
            coef+=4096;
        }
        offsetCalculat+=coef;
    }
    return 0;
}

int main(){

    int fd1,fd2,shm,ok=0,fdFisier,map=0;
    unsigned int offset,section=0,bytes,valori[3];
    off_t size;
    char *sharedArea = NULL;
    char *data=NULL;
    char mS=7,mE=5, mesajS[7]="SUCCESS", mesajE[5]="ERROR";
    if(access(PIPE1,0)==0){
        unlink(PIPE1);
    }

    if(mkfifo(PIPE1,0600)!=0){
        printf("ERROR\ncannot create the response pipe\n");
        return -1;
    }

    fd2=open(PIPE2,O_RDONLY);
    if(fd2==-1){
        printf("ERROR\ncannot open the request pipe\n");
        return -1;
    }

    fd1=open(PIPE1,O_WRONLY);
    if(fd1==-1){
        printf("ERROR\ncannot open the response pipe\n");
        return -1;
    }

    char vec[7]="CONNECT";
    char capacitate=7;
    write(fd1,&capacitate,sizeof(char));
    write(fd1,vec,7*sizeof(char));
    for(;;){
        read(fd2,&capacitate,sizeof(char));
        int cap=capacitate;
        char mesaj[cap+1];
        for(int i=0; i<cap; i++){
            read(fd2,&mesaj[i],sizeof(char));
        }
        mesaj[cap]=0;
        if(strcmp(mesaj,"PING")==0){
            char ping[4]="PING";
            capacitate=4;
            write(fd1,&capacitate,sizeof(char));
            write(fd1,&ping,4*sizeof(char));
            char pong[4]="PONG";
            write(fd1,&capacitate,sizeof(char));
            write(fd1,&pong,4*sizeof(char));
            unsigned int nr= 65781;
            write(fd1,&nr,sizeof(unsigned int));
        }

        if(strcmp(mesaj,"CREATE_SHM")==0){
            unsigned int marime;
            ok=1;
            read(fd2,&marime,sizeof(unsigned int));
            shm = shm_open("/56d67Tj", O_CREAT | O_RDWR, 0644);
            if(shm < 0) {
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,10*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }else{
                ftruncate(shm, marime*sizeof(char));
                sharedArea = (char*)mmap(0, marime*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
                if(sharedArea == (void*)-1){
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,10*sizeof(char));
                    write(fd1,&mE,sizeof(char));
                    write(fd1,&mesajE,5*sizeof(char));
                }else{
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,10*sizeof(char));
                    write(fd1,&mS,sizeof(char));
                    write(fd1,&mesajS,7*sizeof(char));
                }
            }
        }

        if(strcmp(mesaj,"WRITE_TO_SHM")==0){
            unsigned int offset;
            read(fd2,&offset,sizeof(unsigned int));
            if(offset<0 || offset>5195494){
                unsigned int c;
                read(fd2,&c,sizeof(unsigned));
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,12*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }else{
                char c;
                for(int i=0; i<4; i++){
                    read(fd2,&c,sizeof(char));
                    sharedArea[offset+i]=c;
                }
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,12*sizeof(char));
                write(fd1,&mS,sizeof(char));
                write(fd1,&mesajS,7*sizeof(char));
            }
        }

        if(strcmp(mesaj,"MAP_FILE")==0){
            map=1;
            char lungime;
            read(fd2,&lungime,sizeof(char));
            char fisier[lungime+1];
            read(fd2,&fisier,lungime*sizeof(char));
            fisier[(int)lungime]=0;
            fdFisier = open(fisier, O_RDONLY);
            if(fdFisier == -1) {
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,8*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }else{
                size = lseek(fdFisier, 0, SEEK_END);
                lseek(fdFisier, 0, SEEK_SET);
                data = (char*)mmap(NULL, size, PROT_READ ,MAP_PRIVATE, fdFisier, 0);
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,8*sizeof(char));
                write(fd1,&mS,sizeof(char));
                write(fd1,&mesajS,7*sizeof(char));    
            }
        }

        if(strcmp(mesaj,"READ_FROM_FILE_OFFSET")==0){
            //unsigned int offset;
            read(fd2,&offset,sizeof(unsigned int));
            unsigned int lungime;
            read(fd2,&lungime,sizeof(unsigned int));
            if(offset>size || (offset+lungime)>size || data == (void*)-1 || verificareFisierSF(data,size,valori,section)==-1){
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,21*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }else{
                memcpy(sharedArea,(data+offset),lungime);
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,21*sizeof(char));
                write(fd1,&mS,sizeof(char));
                write(fd1,&mesajS,7*sizeof(char));
            }

        }

        if(strcmp(mesaj,"READ_FROM_FILE_SECTION")==0){
            read(fd2,&section,sizeof(unsigned int));
            read(fd2,&offset,sizeof(unsigned int));
            read(fd2,&bytes,sizeof(unsigned int));
            if(verificareFisierSF(data,size,valori,section)==0){
                if((offset+bytes)>valori[1] || section>valori[2]){
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,22*sizeof(char));
                    write(fd1,&mE,sizeof(char));
                    write(fd1,&mesajE,5*sizeof(char));
                }else{
                    memcpy(sharedArea,data+valori[0]+offset,bytes);
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,22*sizeof(char));
                    write(fd1,&mS,sizeof(char));
                    write(fd1,&mesajS,7*sizeof(char));
                }
            }else{
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,22*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }

        }

        if(strcmp(mesaj,"READ_FROM_LOGICAL_SPACE_OFFSET")==0){
            read(fd2,&offset,sizeof(unsigned int));
            read(fd2,&bytes,sizeof(unsigned int));
            if(verificareFisierSFLogical(data,size,offset,valori)==0){
                if((offset-valori[2]+bytes)>valori[1]){
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,30*sizeof(char));
                    write(fd1,&mE,sizeof(char));
                    write(fd1,&mesajE,5*sizeof(char));
                }else{
                    memcpy(sharedArea,data+valori[0]+offset-valori[2],bytes);
                    write(fd1,&capacitate,sizeof(char));
                    write(fd1,&mesaj,30*sizeof(char));
                    write(fd1,&mS,sizeof(char));
                    write(fd1,&mesajS,7*sizeof(char));
                }
            }else{
                write(fd1,&capacitate,sizeof(char));
                write(fd1,&mesaj,30*sizeof(char));
                write(fd1,&mE,sizeof(char));
                write(fd1,&mesajE,5*sizeof(char));
            }

        }

        if(strcmp(mesaj,"EXIT")==0){
            if(ok==1){
                munmap(sharedArea,5195498);
                sharedArea = NULL;
                shm_unlink("/56d67Tj");
            }
            if(map==1){
                close(fdFisier);
                data=NULL;
                munmap(data,size);
            }
            close(fd1);
            close(fd2);
            unlink(PIPE1);
            break;
        }

    }
    
    return 0;
}