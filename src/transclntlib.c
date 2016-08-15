/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */

static char	fpath[PATH_MAX];

int
jitopen(char *path)
{
    int		sz;
    locked_lock(path);
    sz = locked_read(fpath, PATH_MAX);
    if (sz < 0) {
	fprintf(stderr, "Cannot get a file\n");
	return -1;
    }
    open(path, );
}

int
jitclose(int fd)
{
    fd = close(fd);
    locked_unlock();
    return fd;
}
