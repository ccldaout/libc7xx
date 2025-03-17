/*
 * c7gitbr.cpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>


static char *readall(const char *path, ssize_t *size)
{
    struct stat st;
    int fd;

    if ((fd = ::open(path, O_RDONLY)) != -1) {
	ssize_t sizealt;
	if (size == 0)
	    size = &sizealt;
	(void)fstat(fd, &st);
	if (auto p = std::malloc(st.st_size + 1); p != nullptr) {
	    char *buf = static_cast<char*>(p);
	    *size = read(fd, buf, st.st_size);
	    if (*size == st.st_size) {
		close(fd);
		buf[*size] = 0;
		return buf;
	    }
	    std::free(buf);
	}
	close(fd);
    }
    return nullptr;
}

static void print(int prompt, const char *s)
{
    if (prompt) {
	(void)std::fputs(" [", stdout);
    }
    std::fputs(s, stdout);
    if (prompt) {
	(void)std::fputc(']', stdout);
    }
    std::fflush(stdout);
}

static int cdgit(void)
{
    struct stat st_cur;

    ::stat(".", &st_cur);

    for (;;) {
	if (::access(".git", F_OK) == 0) {
	    if (::chdir(".git") == 0) {
		return 1;
	    }
	    char *p, *s = readall(".git", nullptr);
	    if (s == nullptr) {
		return 0;
	    }
	    if (std::strncmp(s, "gitdir: ", 8) != 0) {
		return 0;
	    }
	    s += 8;
	    if ((p = std::strchr(s, '\n')) != nullptr) {
		*p = 0;
	    }
	    if (::chdir(s) == 0) {
		return 1;
	    }
	    return 0;
	}
	auto st_child = st_cur;
	if (::chdir("..") != 0) {
	    return 0;
	}
	if (::stat(".", &st_cur) != 0) {
	    return 0;
	}
	if (st_cur.st_ino == st_child.st_ino) {
	    return 0;
	}
    }
}

int main(int argc, char **argv)
{
    int prompt = 0;
    ssize_t size;
    char *HEAD;

    if (argv[1] && std::strcmp(argv[1], "-p") == 0) {
	prompt = 1;
    }

    if (cdgit() == 0) {
	return 1;
    }

    if ((HEAD = readall("HEAD", &size)) == nullptr) {
	return 1;
    }

    char *p = HEAD + size;
    if (HEAD != p && p[-1] == '\n') {
	*--p = 0;
    }
    for (; HEAD != p; p--) {
	if (*p == '/') {
	    print(prompt, p+1);
	    return 0;
	}
    }
    print(prompt, "???");
    return 0;
}
