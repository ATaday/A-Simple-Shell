#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#define CMD_HISTORY_SIZE 10 // en çok girilen 10 komut için size 10

static char * cmd_history[CMD_HISTORY_SIZE];
static int cmd_history_count = 0;

// kullanıcıdan komutu alıp işlemek için fonksiyon

static void exec_cmd(const char * line)
{
	char * CMD = strdup(line);
	char *params[10];
	int argc = 0;

	// komut parametrelerini ayrıştır

	params[argc++] = strtok(CMD, " ");
	while(params[argc-1] != NULL){	// token aldığı sürece
		params[argc++] = strtok(NULL, " ");
	}

	argc--; // null parametresini de token olarak almaması için argc' yi bir azalt 

	// arka planda çalışma kontrolleri

	int background = 0;
	if(strcmp(params[argc-1], "&") == 0){
		background = 1; // arka planda çalışması için set et
		params[--argc] = NULL;	// params arrayinden "&" yi sil
	}

	int fd[2] = {-1, -1};	// input ve output için file descriptors 

	while(argc >= 3){

		// yönlendirme parametre kontrolü

		if(strcmp(params[argc-2], ">") == 0){	// output

			// dosyayı aç
			// open fonksiyonu parametreleri ile ilgili link: http://pubs.opengroup.org/onlinepubs/7908799/xsh/open.html

			fd[1] = open(params[argc-1], O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP|S_IWGRP);
			if(fd[1] == -1){
				perror("open");
				free(CMD);
				return;
			}

			// parametre arrayini güncelle

			params[argc-2] = NULL;
			argc -= 2;
		}else if(strcmp(params[argc-2], "<") == 0){ // input
			fd[0] = open(params[argc-1], O_RDONLY);
			if(fd[0] == -1){
				perror("open");
				free(CMD);
				return;
			}
			params[argc-2] = NULL;
			argc -= 2;
		}else{
			break;
		}
	}

	int status;
	pid_t pid = fork();	// yeni bir işlem oluştur
	switch(pid){
		case -1:	// hata
			perror("fork");
			break;
		case 0:	// child
			if(fd[0] != -1){	// yönlendirme varsa
				if(dup2(fd[0], STDIN_FILENO) != STDIN_FILENO){
					perror("dup2");
					exit(1);
				}
			}
			if(fd[1] != -1){	// yönlendirme varsa
				if(dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO){
					perror("dup2");
					exit(1);
				}
			}
			execvp(params[0], params);
			perror("execvp");
			exit(0);
		default: // parent
			close(fd[0]);close(fd[1]);
			if(!background)
				waitpid(pid, &status, 0);
			break;
	}
	free(CMD);
}

// history' e komut eklemek için fonksiyon

static void add_to_history(const char * cmd){
	if(cmd_history_count == (CMD_HISTORY_SIZE-1)){	// history dolu ise
		int i;
		free(cmd_history[0]);	// historydeki ilk komutu adresten sal
		
		// diğer komutları bir indis shift et

		for(i=1; i < cmd_history_count; i++)
			cmd_history[i-1] = cmd_history[i];
		cmd_history_count--;
	}
	cmd_history[cmd_history_count++] = strdup(cmd);	// komutu history' e ekle
}

// history' den komutları çalıştırmak için fonksiyon

static void run_from_history(const char * cmd){
	int index = 0;
	if(cmd_history_count == 0){
		printf("No commands in history\n");
		return ;
	}

	if(cmd[1] == '!') // ikinci karakter '!' ise son girilen komutun indeksini al
		index = cmd_history_count-1;
	else{
		index = atoi(&cmd[1]) - 1;	// ikinci karakteri history indeksi için kullanıcıdan al
		if((index < 0) || (index > cmd_history_count)){ // history' de böyle bir indis yok ise hata bastır
			fprintf(stderr, "No such command in history.\n");
			return;
		}
	}
	printf("%s\n", cmd);	// hangi indis komutunun girildiğini bas
	exec_cmd(cmd_history[index]);	// o indisteki komutu çalıştır
}

// history bufferındaki öğeleri bastırmak için fonksiyon

static void list_history(){
	int i;
	for(i=cmd_history_count-1; i >=0 ; i--){	// her history öğesi için
		printf("%i %s\n", i+1, cmd_history[i]);
	}
}

// sinyal işlemleri için fonksiyon

static void signal_handler(const int rc){
	switch(rc){
		case SIGTERM:
		case SIGINT:
			break;
		
		case SIGCHLD:
			
			 /* tüm ölü işlemler için bekle ve bir child programın başka bir bölümünde bitirilirse 
			 signal handlerin işlemi bloke etmemesi için non-blocking çağrı kullan */
			
			while (waitpid(-1, NULL, WNOHANG) > 0);
			break;
	}
}

// main

int main(int argc, char *argv[]){

	// sinyalleri yakala

	struct sigaction act, act_old;
	act.sa_handler = signal_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if(	(sigaction(SIGINT,  &act, &act_old) == -1) ||
		(sigaction(SIGCHLD,  &act, &act_old) == -1)){ // Ctrl^C
		perror("signal");
		return 1;
	}

	// line için buffer ayır 

	size_t line_size = 100;
	char * line = (char*) malloc(sizeof(char)*line_size);
	if(line == NULL){	// malloc falso verirse hata ver
		perror("malloc");
		return 1;	
	}

	int inter = 0; // satır alma işlemleri için flag tut
	while(1){
		if(!inter)
			printf("mysh > ");
		if(getline(&line, &line_size, stdin) == -1){	// input almak için satır oku
			if(errno == EINTR){	 // errno hata kodu olursa 
				clearerr(stdin); // error stringini temizle
				inter = 1;	 // flagi 1' e set et
				continue; // aşağı inmeden loopa tekrar dön
			}
			perror("getline");
			break;
		}
		inter = 0; // flagi tekrar sıfırla

		int line_len = strlen(line);
		if(line_len == 1){	// sadece yeni satır girilirse
			continue;
		}
		line[line_len-1] = '\0'; // yeni satırı sil

		
		if(strcmp(line, "exit") == 0){ // girilen satır "exit" ise döngüden çıkıp programı sonlandır
			break;
		}else if(strcmp(line, "history") == 0){ // girilen satır "history" ise list_history fonksiyonunu çağır
			list_history();
		}else if(line[0] == '!'){ // girilen satırın ilk karakter "!" karakteri ise run_from_history fonksiyonunu çağır
			run_from_history(line);
		}else{ // yukarıdaki durumlara girmezse
			add_to_history(line); // girilen satırı history' e eklemek için add_to_history fonksiyonunu çağır
			exec_cmd(line); // yeni satırı almak için exec_cmd fonksiyonunu çağır
		}
	}

	free(line);
	return 0;
}
