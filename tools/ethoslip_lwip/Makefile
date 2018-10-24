
CC = gcc
STRIP = strip
AR = ar

OBJ = ethoslip_lwip

CRU_PATH := .

FILE_LIST = \
$(CRU_PATH)/user/*.c \
$(CRU_PATH)/src/*.c \
$(CRU_PATH)/lwip-2.1.0/netif/ethernet.c \
$(CRU_PATH)/lwip-2.1.0/arch/*.c \
$(CRU_PATH)/lwip-2.1.0/core/*.c \
$(CRU_PATH)/lwip-2.1.0/core/ipv4/*.c \
$(CRU_PATH)/lwip-2.1.0/core/ipv6/*.c


INC_LIST = \
-I $(CRU_PATH)/lwip-2.1.0/include \
-I $(CRU_PATH)/lwip-2.1.0/arch \
-I $(CRU_PATH)/lwip-2.1.0 \
-I $(CRU_PATH)/src

LIB_NAME = \
-lpthread


all:
	$(CC) -Wall $(FILE_LIST) $(INC_LIST) $(LIB_NAME)  -o $(OBJ) -std=gnu99
	$(STRIP) $(OBJ)

clean:
	rm -f $(OBJ)
