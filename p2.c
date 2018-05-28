#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdbool.h>
#include<time.h>

uint32_t lowest32bitnum=2^32;
uint32_t lowest31bitnum=2^31;

union block32{
	uint32_t block;
	char c[4];
};

//https://en.wikipedia.org/wiki/Modular_exponentiation#Right-to-left_binary_method
uint64_t modexp(uint64_t base,uint64_t exponent,uint64_t modulus){
	if(modulus==1) return 0;
	uint64_t result=1;
	
	base=base%modulus;

	while(exponent>0){
		if(exponent%2==1){
			result=(result*base)%modulus;
		}
		exponent=exponent>>1;
		base=base*base%modulus;
	}

	return result;	

}

//https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
int miller_rabin(uint64_t n, int k){
	//get r and d;
	uint64_t d=n-1;
	uint64_t r=0;
	while(d%2==0){
		d=d/2;
		r++;
	}

	//printf("2^%lu*%lu=%lu-1\n",r,d,n);

	for(int i=0;i<k;i++){
		uint64_t a=((uint64_t)rand())%n-1;

		if(a<2){
			a+=2;
		}

		//printf("a=%d\n",a);

		uint64_t x=modexp(a,d,n);

		if(x==1||x==n-1){
			continue;
		}

		int continuewitness=0;

		for(int j=0;j<r-1;j++){
			x=modexp(x,2,n);
			if(x==1){
				return 0;
			}
			if(x==n-1){
				continuewitness=1;
				break;
			}
		}

		if(continuewitness==1){
			continue;
		}

		return 0;

	}

	return 1;
}

void encrypt(FILE* ptext, uint64_t p, uint64_t g, uint64_t e2){

	FILE* ctext=fopen("ctext.txt","w");	

	int done=0;
	int notext=0;

	while(done!=1){


		uint64_t k=rand()%p;

		union block32 m;
		m.block=0;

		//printf("k=%lu\n",k);

		for(int i=0;i<4;i++){
			char c=fgetc(ptext);

			if(c!=EOF){
				m.c[3-i]=c;
			}else{
				done=1;
				if(i==0){
					notext=1;
				}
			}

		}
		if(notext==0){

			uint64_t c1=modexp(g,k,p);

			uint64_t c2p1=modexp(e2,k,p);
			uint64_t c2=(m.block*c2p1)%p;

			//printf("m=%x\n",m.block);

			printf("%lu %lu\n",c1,c2);
			fprintf(ctext,"%lu %lu\n",c1,c2);
		}
	}

	fclose(ctext);

}

void decrypt(FILE* ctext,uint64_t p, uint64_t g, uint64_t d){
	FILE* ptext=fopen("ptext.txt","w");
	int done=0;

	while(done==0){
		uint64_t c1=0;
		uint64_t c2=0;
		char buffer[512]={'\0'};
		int i=0;
		char c;
		while(true){
			c=fgetc(ctext);
			if(c=='\n'){
				break;
			}else if(c=='\0'||c==EOF){
				done=1;
				break;
			}else{
				buffer[i]=c;
				i++;
			}


		}

		if(sscanf(buffer,"%lu %lu",&c1,&c2)==2){

			//printf("c1=%lu c2=%lu\n",c1,c2);

			uint32_t m=(modexp(c1,p-1-d,p)*(c2%p))%p;
			union block32 block;
			block.block=m;
			printf("%c%c%c%c",block.c[3],block.c[2],block.c[1],block.c[0]);

			for(i=3;i>-1;i--){
				fputc(block.c[i],ptext);
			}


		}
	}

	printf("\n");

	fclose(ptext);

}

void keygen(){

	printf("Input Seed:\n");
	int seed=0;
	scanf("%d",&seed);

	//printf("%d\n",seed);

	srand(seed);

	FILE* prikey=fopen("prikey.txt","w");
	FILE* pubkey=fopen("pubkey.txt","w");

	uint32_t q;
	uint32_t p;
	uint32_t g=2;
	uint32_t d;
	uint32_t e2;
	int p_isvalid=0;
	int p_isprime=0;

	while(!p_isvalid){
		q=rand();
		q=q|0x40000001;

		if(miller_rabin(q,64)==1&&q%12==5){
			if(miller_rabin((2*q)+1,64)==1){
				p=(2*q)+1;
				break;
			}
		}

		

	}

	//printf("q=%u p=%u\n",q,p);

	d=rand()%p;

	e2=modexp(g,d,p);

	printf("pubkey: %u %u %u\n",p,g,e2);
	printf("prikey: %u %u %u\n",p,g,d);

	fprintf(pubkey,"%u %u %u",p,g,e2);
	fprintf(prikey,"%u %u %u",p,g,d);

	fclose(pubkey);
	fclose(prikey);
	
	
}

int main(int argc, char* argv[]){
	srand(time(NULL));
	
	if(argc==1){

		printf("pass args -k to generate keys,-e to encrypt, or -d to decrypt\n");

	}else if(argc==2){
		
		if(strcmp(argv[1],"-d")==0){
			printf("decrypting:\n\n");
			
			FILE* prikey=fopen("prikey.txt","r");
			
			char buffer[512]={'\0'};
			char c;

			int i=0;
			while((c=fgetc(prikey))!=EOF){
				buffer[i]=c;
				i++;
			}

			//printf("prikey=%s\n",buffer);
			
			uint64_t p=0;
			uint64_t g=0;
			uint64_t d=0;

			sscanf(buffer,"%lu %lu %lu",&p,&g,&d);

			//printf("p=%lu g=%lu d=%lu\n",p,g,d);

			FILE* ctext=fopen("ctext.txt","r");

			decrypt(ctext,p,g,d);

			fclose(ctext);

			fclose(prikey);

		}else if(strcmp(argv[1],"-e")==0){
			printf("encrypting:\n\n");

			FILE* pubkey=fopen("pubkey.txt","r");

			char buffer[512]={'\0'};
			char c;

			int i=0;
			while((c=fgetc(pubkey))!=EOF){
				buffer[i]=c;
				i++;
			}

			//printf("pubkey=%s\n\n",buffer);
			
			uint64_t p=0;
			uint64_t g=0;
			uint64_t e2=0;

			sscanf(buffer,"%lu %lu %lu",&p,&g,&e2);

			//printf("p=%lu g=%lu e2=%lu\n",p,g,e2);

			FILE* ptext=fopen("ptext.txt","r");

			encrypt(ptext,p,g,e2);
		}else if(strcmp(argv[1],"-k")==0){
			printf("generating key:\n");
			keygen();
		}
	}

	//debug mod exp
	/*
	for(int i=0;i<32;i++){

		printf("%lu\n",modexp(2,i,10));

	}*/

	//debug miller-rabin

	//printf("is prime: %d\n",miller_rabin(452930481,64));
	
	return 0;
}
