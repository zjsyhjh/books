#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define EPOLL_SIZE 100
#define BUF_SIZE 1024

class process {
public:
	process() : m_pid(-1) {}

public:
	pid_t m_pid;
	int m_pipefd[2];
};


template<typename T >
class processpool {
private:
	processpool(int listenfd, int process_number = 8);
public:
	static processpool<T >* create(int listenfd, int process_number = 8) {
		if (!m_instance) {
			m_instance = new processpool<T >(listenfd, process_number);
		}
		return m_instance;
	}
	~processpool() {
		delete []m_sub_process;
	}
	void run();
private:
	void setup_sig_pipe();
	void run_parent();
	void run_child();
private:
	static processpool<T >* m_instance;
	static const int MAX_PROCESS_NUMBER = 16;
	static const int USER_PER_PROCESS = 65536;
	static const int MAX_EVENT_NUMBER = 10000;
private:
	process* m_sub_process;
	int m_process_number;
	int m_index;
	int m_epollfd;
	int m_listenfd;
	int m_stop;
};

template<typename T>
processpool<T >* processpool<T >::m_instance = NULL;

static int sig_pipefd[2];

static int setnonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

static void addfd(int epollfd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


static void sig_handler(int sig) {
	int save_errno = errno;
	int msg = sig;
	send(sig_pipefd[1], (char *)&msg, 1, 0);
	errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	if (restart) {
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

template<typename T>
processpool<T >::processpool(int listenfd, int process_number)
	:m_listenfd(listenfd), m_process_number(process_number), m_index(-1), m_stop(false)
{
	assert(m_process_number > 0 && m_process_number <= MAX_PROCESS_NUMBER);

	m_sub_process = new process[process_number];
	assert(m_sub_process);

	for (int i = 0; i < process_number; i++) {
		int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
		assert(ret == 0);

		m_sub_process[i].m_pid = fork();
		assert(m_sub_process[i].m_pid >= 0);

		if (m_sub_process[i].m_pid > 0) {
			//parent process
			close(m_sub_process[i].m_pipefd[1]);
			continue;
		} else {
			//child process
			close(m_sub_process[i].m_pipefd[0]);
			m_index = i;
			break;
		}
	}
}


template<typename T >
void processpool<T >::setup_sig_pipe() {
	m_epollfd = epoll_create(EPOLL_SIZE);
	assert(m_epollfd != -1);

	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
	assert(ret != -1);

	setnonblocking(sig_pipefd[1]);
	addfd(m_epollfd, sig_pipefd[0]);

	addsig(SIGCHLD, sig_handler);
	addsig(SIGTERM, sig_handler);
	addsig(SIGINT, sig_handler);
	addsig(SIGPIPE, SIG_IGN);
}

template<typename T>
void processpool<T >::run() {
	if (m_index != -1) {
		run_child();
	} else {
		run_parent();
	}
}

template<typename T>
void processpool<T >::run_child() {
	setup_sig_pipe();
	int pipefd = m_sub_process[m_index].m_pipefd[1];

	addfd(m_epollfd, pipefd);

	epoll_event events[MAX_EVENT_NUMBER];
	T *users = new T[USER_PER_PROCESS];
	assert(users);

	while (!m_stop) {
		int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
		if ((number < 0) && (errno != EINTR)) {
			printf("epoll_wait() error\n");
			break;
		}

		for (int i = 0; i < number; i++) {
			int sockfd = events[i].data.fd;
			if ((sockfd == pipefd) && (events[i].events & EPOLLIN)) {
				int client = 0;
				int ret = recv(sockfd, (char *)&client, sizeof(client), 0);
				if (((ret < 0) && (errno != EAGAIN)) || ret == 0) {
					continue;
				} else {
					struct sockaddr_in clnt_addr;
					socklen_t clnt_sz = sizeof(clnt_addr);
					int connfd = accept(m_listenfd, (struct sockaddr*)&clnt_addr, &clnt_sz);
					if (connfd < 0) {
						printf("accept() errno is : %d\n", errno);
						continue;
					}
					addfd(m_epollfd, connfd);
					users[connfd].init(m_epollfd, connfd, clnt_addr);
				}
			} else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
				char signals[BUF_SIZE];
				int ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
				if (ret <= 0) {
					continue;
				} else {
					for (int j = 0; j < ret; j++) {
						switch(signals[j]) {
							case SIGCHLD:
								pid_t pid;
								int status;
								while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
									continue;
								}
								break;
							case SIGTERM:
							case SIGINT:
								m_stop = true;
								break;
							default:
								break;
						}
					}
				}
			} else if (events[i].events & EPOLLIN) {
				users[sockfd].process();
			}
		}
	}
	delete []users;
	users = NULL;
	close(pipefd);
	close(m_epollfd);
}


template<typename T>
void processpool<T >::run_parent() {
	setup_sig_pipe();

	addfd(m_epollfd, m_listenfd);

	epoll_event events[MAX_EVENT_NUMBER];
	int sub_process_counter = 0;

	while (!m_stop) {
		int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
		if ((number < 0) && (errno != EINTR)) {
			printf("epoll_wait() error\n");
			break;
		}

		for (int i = 0; i < number; i++) {
			int sockfd = events[i].data.fd;
			if (sockfd == m_listenfd) {

				int k =sub_process_counter;
				do {
					if (m_sub_process[k].m_pid != -1) {
						break;
					}
					k = (k + 1) % m_process_number;
				} while (k != sub_process_counter);

				if (m_sub_process[k].m_pid == -1) {
					m_stop = true;
					break;
				}

				sub_process_counter = (k + 1) % m_process_number;
				int new_conn = 1;
				send(m_sub_process[k].m_pipefd[0], (char*)&new_conn, sizeof(new_conn), 0);
				printf("send request to child %d\n", k);
			} else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
				char signals[BUF_SIZE];
				int ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
				if (ret <= 0) {
					continue;
				} else {
					for (int j = 0; j < ret; j++) {
						switch(signals[j]) {
							case SIGCHLD:
								pid_t pid;
								int status;
								while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
									for (int k = 0; k < m_process_number; k++) {
										if (m_sub_process[k].m_pid == pid) {
											printf("child %d join\n", k);
											close(m_sub_process[k].m_pipefd[0]);
											m_sub_process[k].m_pid = -1;
										}
									}
								}
								m_stop = true;
								for (int k = 0; k < m_process_number; k++) {
									if (m_sub_process[k].m_pid != -1) {
										m_stop = false;
									}
								}
								break;
							case SIGTERM:
							case SIGINT:
								printf("kill all children now\n");
								for (int k = 0; k < m_process_number; k++) {
									int pid = m_sub_process[k].m_pid;
									if (pid != -1) {
										kill(pid, SIGTERM);
									}
								}
								break;
							default:
								break;
						}
					}
				}
			}
		}
	}
	close(m_epollfd);
}




#endif