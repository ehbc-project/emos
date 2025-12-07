# syscalls

```c
typedef long dir_handle_t;
typedef long file_handle_t;
typedef long dev_handle_t;
typedef long dev_if_handle_t;
typedef long timer_handle_t;
typedef long sys_power_state_t;
typedef long mode_t;
typedef long pid_t;

struct directory_entry;
struct file_info;

/***************************************
 * Native System Calls
 */

int proc_terminate(void);               // syscall 0x00000000
int proc_execute(                       // syscall 0x00000001
    IN const char *path,
    IN char *const argv[],
    IN char *const envp[]
);
int proc_execute_fromdir(               // syscall 0x00000002
    IN dir_handle_t dh, 
    IN const char *path,
    IN char *const argv[],
    IN char *const envp[],
    IN int flags
);
int proc_fork(                          // syscall 0x00000003
    OUT pid_t pid
);
int proc_change_cwd(                    // syscall 0x00000004
    IN const char *path
);
int proc_change_cwd_fromdir(            // syscall 0x00000005
    IN dir_handle_t dh
);

int fs_mount(                           // syscall 0x00010000
    IN const char *type,
    IN const char *name,
    IN int flags,
    IN OPT void *fs_args
)
int fs_unmount(                         // syscall 0x00010001
    IN const char *name,
    IN int flags
)

int dir_open(                           // syscall 0x00020000
    OUT dir_handle_t *dh,
    IN const char *path,
    IN int flags,
    IN mode_t mode
);
int dir_open_fromdir(                   // syscall 0x00020001
    OUT dir_handle_t *newdh,
    IN dir_handle_t dh,
    IN const char *path,
    IN int flags,
    IN mode_t mode
);
int dir_read(                           // syscall 0x00020002
    IN dir_handle_t dh,
    OUT struct directory_entry *dirent
);
int dir_close(                          // syscall 0x00020003
    IN dir_handle_t dh
);

int file_open(                          // syscall 0x00030000
    OUT file_handle_t *fh,
    IN const char *path,
    IN int flags,
    IN mode_t mode
);
int file_open_fromdir(                  // syscall 0x00030001
    OUT file_handle_t *fh,
    IN dir_handle_t dh,
    IN const char *path,
    IN int flags,
    IN mode_t mode
);
int file_getinfo(                       // syscall 0x00030002
    OUT struct file_info *buf,
    IN file_handle_t fh
);
int file_read(                          // syscall 0x00030003
    OUT size_t *read_count,
    IN file_handle_t fh,
    OUT void *buf,
    IN size_t count
);
int file_write(                         // syscall 0x00030004
    OUT size_t *written_count,
    IN file_handle_t fh,
    IN const void *buf,
    IN size_t count
);
int file_seek(                          // syscall 0x00030005
    OUT loff_t *result,
    IN file_handle_t fh,
    IN loff_t offset,
    IN int whence
);
int file_sync(                          // syscall 0x00030006
    IN file_handle_t fh
);
int file_flush(                         // syscall 0x00030007
    IN file_handle_t fh
);
int file_lock(                          // syscall 0x00030008
    IN file_handle_t fh,
    IN int flags
);
int file_unlock(                        // syscall 0x00030009
    IN file_handle_t fh
);
int file_close(                         // syscall 0x0003000A
    IN file_handle_t fh
);

int dev_open(                           // syscall 0x00040000
    OUT dev_handle_t *dh,
    IN const char *name,
    IN int flags, 
    IN mode_t mode
);
int dev_if_find(                        // syscall 0x00040001
    OUT dev_if_handle_t *dih,
    IN dev_handle_t dh,
    IN const char *name
);
int dev_if_exec(                        // syscall 0x00040002
    OUT void *result,
    IN dev_handle_t dh,
    IN dev_if_handle_t dih,
    IN long func,
    ...
);
int dev_close(                          // syscall 0x00040003
    IN dev_handle_t dh
);

int mem_map(                            // syscall 0x00050000
    OUT void *result,
    IN void *addr,
    IN size_t len,
    IN int prot,
    IN int flags,
    IN int flides,
    IN off_t offset
);
int mem_unmap(                          // syscall 0x00050001
    IN void *addr,
    IN size_t len
);

int time_get_utc_time(                  // syscall 0x00060000
    OUT time_t *time
);
int time_get_local_time(                // syscall 0x00060001
    OUT time_t *time
);
int time_get_time_of_day(               // syscall 0x00060002
    OUT struct time_value *time
);
int time_set_time_of_day(               // syscall 0x00060003
    IN const struct time_value *time
);
int time_get_date(                      // syscall 0x00060004
    OUT struct date_value *date
);
int time_set_date(                      // syscall 0x00060005
    IN const struct date_value *date
);

int sched_yield();                      // syscall 0x00070000


int sys_reboot(                         // syscall 0x00080000
    IN OPT const char *msg
);
int sys_set_power_state(                // syscall 0x00080001
    IN sys_power_state_t pstate
);

/***************************************
 * POSIX-Compatible Subsystem System Calls
 */

int posix_accept(int s, struct sockaddr *addr, int *addrlen);
int posix_access(const char 8path, int mode);
int posix_bind(int s, const struct sockaddr *name, int namelen);
int posix_chdir(const char *path);
int posix_chmod(const char *path, mode_t mode);
int posix_chown(const char *path, uid_t owner, gid_t group);
int posix_chroot(const char *dirname);
int posix_close(int flides);
int posix_connect(int s, const struct sockaddr *name, int namelen);
int posix_dup(int flides);
int posix_dup2(int flides, int flides2);
int posix_fchdir(int fd);
int posix_fchmod(int fd, mode_t mode);
int posix_fchown(int fd, uid_t owner, gid_t group);
int posix_fcntl(int flides, int cmd, ...);
int posix_flock(int fd, int operation);
int posix_fpathconf(int fd, int name);
int posix_fstat(int flides, struct stat *buf);
int posix_fstatfs(int fd, struct statfs *buf);
int posix_fsync(int fd);
int posix_ftruncate(int fd, off_t length);
int posix_getdirentries(int fd, char *buf, int nbytes,long *basep);
int posix_getdomainname(char *name, int namelen);
int posix_getfh(char *path, fhandle_t *fhp);
int posix_getfsstat(struct statfs *buf, long bufsize, int flags);
int posix_gethostname(char *hostname, size_t len);
int posix_getpeername(int s, struct sockaddr *name, int *namelen);
int posix_getrlimit(int resource, struct rlimit *rlp);
int posix_getsockname(int s, struct sockaddr *name, int *namelen);
int posix_getsockopt(int s, int level, int optname, void *optval, int *optlen);
int posix_gettimeofday(struct timeval *tp, struct timezone *tzp);
int posix_ioctl(int flides, int request, ...);
int posix_link(const char *name1, const char *name2);
int posix_listen(int s, int backlog);
int posix_lseek(int flides, off_t offset, int whence);
int posix_lstat(const char *path, struct stat *buf);
int posix_mkdir(const char *path, mode_t mode);
int posix_mkfifo(const char *path, mode_t mode);
int posix_mknod(const char *path, mode_t mode, dev_t dev);
int posix_mmap(void *addr, size_t len, int prot, int flags, int flides, off_t off);
int posix_mount(int type, const char *dir, int flags, caddr_t data);
int posix_mq_close();
int posix_mq_getattr();
int posix_mq_open();
int posix_mq_receive();
int posix_mq_send();
int posix_mq_setattr();
int posix_mq_unlink();
int posix_munmap();
int posix_nfssvc();
int posix_open();
int posix_pipe();
int posix_read();
int posix_readlink();
int posix_readv();
int posix_recv();
int posix_recvfrom();
int posix_recvmsg();
int posix_rename();
int posix_rmdir();
int posix_select();
int posix_send();
int posix_sendmsg();
int posix_sendto();
int posix_setdomainname(const char *name, int namelen);
int posix_sethostname(char *name, int namelen);
int posix_setrlimit(int resource, const struct rlimit *rlp);
int posix_setsockopt(int s, int level, int optname, const void *optval, int optlen);
int posix_settimeofday(struct timeval *tp, struct timezone *tzp);
int posix_shm_open();
int posix_shm_unlink();
int posix_shutdown();
int posix_socket();
int posix_socketpair();
int posix_stat(const char *path, struct stat *buf);
int posix_statfs(const char *path, struct statfs *buf);
int posix_swapon();
int posix_symlink();
int posix_sync();
int posix_truncate(const char *path, off_t length);
int posix_umask();
int posix_unlink();
int posix_unmount(const char *dir, int flags);
int posix_utimes();
int posix_write();
int posix_writev();

```
