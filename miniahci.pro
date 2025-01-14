TEMPLATE = lib

CONFIG -= qt

DEFINES += \
    __KERNEL__ \
    KBUILD_MODNAME=\"\\\"\\\"\"

SOURCES += \
    ahci.c \
    main.c \
    ioctl.c

HEADERS += \
    ahci.h \
    driver.h \
    ioctl.h \
    minipci.h

OTHER_FILES += \
    Makefile

KERNEL_RELEASE = $$system(uname -r)

INCLUDEPATH += \
    /usr/lib/modules/$${KERNEL_RELEASE}/build/include \
    /usr/lib/modules/$${KERNEL_RELEASE}/build/arch/x86/include
