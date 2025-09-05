#ifndef __COMMAND_H__
#define __COMMAND_H__

int volinfo_handler(int argc, char **argv);
int mkdir_handler(int argc, char **argv);
int copy_handler(int argc, char **argv);
int rename_handler(int argc, char **argv);
int lsfile_handler(int argc, char **argv);
int setattr_handler(int argc, char **argv);
int getattr_handler(int argc, char **argv);

#endif // __COMMAND_H__
