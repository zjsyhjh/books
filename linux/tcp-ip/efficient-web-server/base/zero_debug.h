#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M "\n", ##__VA_ARGS__); /* exit(1); */ }

#define check_exit(A, M, ...) if(!(A)) { log_err(M "\n", ##__VA_ARGS__); exit(1);}

#define check_debug(A, M, ...) if(!(A)) { debug(M "\n", ##__VA_ARGS__); /* exit(1); */}

#endif