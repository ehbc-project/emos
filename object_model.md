# Object Model

```c
enum exec_object_type {
    OT_GENERIC = 0,
    OT_SERVICE,
    OT_BUS_DRIVER,
    OT_DEVICE_DRIVER,
    OT_FS_DRIVER,
};

struct exec_object {
    struct guid id;
    char *name;
    enum object_type type;
};
```
