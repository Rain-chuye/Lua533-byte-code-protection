LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# 通用编译选项
LOCAL_CFLAGS   := -std=c99
LOCAL_CFLAGS   += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast   -O3

# 头文件搜索路径
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../lua

# 源文件
LOCAL_SRC_FILES := luajava.c

# 系统动态库
LOCAL_LDLIBS    := -lz -llog -ldl

# 静态库（liblua 和 rand 必须已用 BUILD_STATIC_LIBRARY 构建）
LOCAL_STATIC_LIBRARIES := liblua #rand c++_static

LOCAL_MODULE := luajava

include $(BUILD_SHARED_LIBRARY)
