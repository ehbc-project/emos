# Driver Model

```c
int __driver_get_api_version(void);

int driver_init(int argc, char **argv, char **envp);
int driver_deinit(void);

int driver_probe(struct device *dev, )

```
