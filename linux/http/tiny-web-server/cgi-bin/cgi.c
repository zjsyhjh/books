/* 
 * cgi.c - a minimal CGI program that adds two numbers together
 */
#include "csapp.h"

int main(void) {

	/* Extract the two arguments */
	char *buf;
	int n1 = 0, n2 = 0;
	if ((buf = getenv("QUERY_STRING")) != NULL) {
		char *p = strchr(buf, '&');
		*p = '\0';
		char arg1[MAXLINE], arg2[MAXLINE];
		strcpy(arg1, buf);
		strcpy(arg2, p + 1);
		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}

	/* Make the response body */
	char content[MAXLINE];
	sprintf(content, "Welcome to add.com: ");
	sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
	sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
	sprintf(content, "%sThanks for visiting!\r\n", content);

	/* Generate the HTTP response */
	printf("Connectio: close\r\n");
	printf("Content-length: %d\r\n", (int)strlen(content));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s", content);
	fflush(stdout);
	return 0;
}