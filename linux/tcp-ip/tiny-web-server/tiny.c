#include "csapp.h"


void read_requesthdrs(rio_t *rp) {
	char buf[MAXLINE];
	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
	if (!strstr(uri, "cgi-bin")) {
		/* Static content */
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri) - 1] == '/') {
			strcat(filename, "home.html");
		}
		return 1;
	} else {
		/* Dynamic content */
		char *ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		} else {
			strcpy(cgiargs, "");
			strcat(filename, uri);
		}
		return 0;
	}
}

void get_filetype(char *filename, char *filetype) {
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif")) 
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpg");
	else
		strcpy(filetype, "text/plain");
}

void serve_static(int fd, char *filename, int filesize) {
	char filetype[MAXLINE], buf[MAXLINE];
	/* Send response headers to client */
	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 Ok\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));
	printf("Response headers:\n");
	printf("%s", buf);

	/* Send response body to client */
	int srcfd = Open(filename, O_RDONLY, 0);
	char *srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	Close(srcfd);
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0) {
		/* Child process */
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}
	/* parent waits for and reaps child */
	wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
	char buf[MAXLINE], body[MAXLINE];
	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</tile>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);


	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length : %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}

void doit(int fd) {
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;
	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not implement", "Tiny does not implement this method");
		return;
	}

	read_requesthdrs(&rio);

	/* Parse URI from GET request */
	struct stat sbuf;
	char filename[MAXLINE], cgiargs[MAXLINE];
	int is_static = parse_uri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) { 
		clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
		return;
	}

	if (is_static) {
		/* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);
	} else {
		/* Serve dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	char hostname[MAXLINE], port[MAXLINE];

	int listenfd = Open_listenfd(argv[1]);

	while (1) {
		struct sockaddr_storage clnt_addr;	
		socklen_t clnt_sz

		 = sizeof(clnt_addr);

		int connfd = Accept(listenfd, (SA *)&clnt_addr, &clnt_sz);
		Getnameinfo((SA *)&clnt_addr, clnt_sz, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accept connection from (%s, %s)\n", hostname, port);
		doit(connfd);
		Close(connfd);
	}
	return 0;
}
