/*
 * c7prompt.cpp
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
	::fstat(fd, &st);
	if (auto p = std::malloc(st.st_size + 1); p != nullptr) {
	    char *buf = static_cast<char*>(p);
	    *size = read(fd, buf, st.st_size);
	    if (*size == st.st_size) {
		::close(fd);
		buf[*size] = 0;
		return buf;
	    }
	    std::free(buf);
	}
	::close(fd);
    }
    return nullptr;
}

static void print(const char *curr_dir, const char *base_dir, const char *branch)
{
    char buff[std::strlen(curr_dir) + std::strlen(branch ? branch : "") + 32];

    size_t m = 40;
    if (auto e = std::getenv("C7_PROMPT_MAX"); e != nullptr) {
	if ((m = std::strtol(e, nullptr, 0)) < 24) {
	    m = 24;
	}
    }

    size_t n = std::strlen(base_dir);
    if (std::strncmp(curr_dir, base_dir, n) == 0) {
	if (branch != nullptr) {
	    std::sprintf(buff, "~%s [%s] ", curr_dir+n, branch);
	} else {
	    std::sprintf(buff, "~%s ", curr_dir+n);
	}
    } else {
	const char *p = std::strrchr(curr_dir, '/');
	if (p != nullptr && p != curr_dir && p[1] == 0) {
	    for (p--; curr_dir != p && *p != '/'; p--);
	}
	if (p == nullptr) {
	    p = curr_dir;
	} else {
	    p++;
	}
	if (branch != nullptr) {
	    std::sprintf(buff, "(%s)%% [%s] ", p, branch);
	} else {
	    std::sprintf(buff, "(%s)%% ", p);
	}
    }
    
    if ((n = std::strlen(buff)) > m) {
	n -= m;
	buff[n] = buff[n+1] = buff[n+2] = '.';
    } else {
	n = 0;
    }
    std::fputs(&buff[n], stdout);
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
    ssize_t size;
    char *HEAD;

    if (argc == 1 || argc > 3) {
	std::fprintf(stderr, "Usage: c7prompt CURRENT_DIR [BASE_DIR]\n");
	std::exit(1);
    }
    const char *curr_dir = argv[1];
    const char *base_dir = argv[2];
    if (base_dir == nullptr && (base_dir = getenv("HOME")) == nullptr) {
	base_dir = "";
    }

    if (cdgit() == 0 ||
	(HEAD = readall("HEAD", &size)) == nullptr) {
	print(curr_dir, base_dir, nullptr);
	return 0;
    }

    char *p = HEAD + size;
    if (HEAD != p && p[-1] == '\n') {
	*--p = 0;
    }
    for (; HEAD != p; p--) {
	if (*p == '/') {
	    print(curr_dir, base_dir, p+1);
	    return 0;
	}
    }
    print(curr_dir, base_dir, "?");
    return 0;
}
