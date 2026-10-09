# Makefile para validacao e testes das bibliotecas STM32F7 BSP

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
INCLUDES = -ICommon \
           -IDrivers/BspUart \
           -IDrivers/BspAdc \
           -IDrivers/BspDac \
           -IDrivers/BspSync \
           -IBsp \
           -ITemplates

SRCS = Drivers/BspUart/BspUart.c \
       Drivers/BspAdc/BspAdc.c \
       Drivers/BspDac/BspDac.c \
       Drivers/BspSync/BspSync.c \
       Bsp/Bsp.c \
       Templates/LibModel.c \
       Examples/main_example.c

OBJS = $(SRCS:.c=.o)
TARGET = bsp_demo

.PHONY: all clean format doc test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS)
	@echo "=========================================="
	@echo " Build concluido com sucesso! (Zero erros)"
	@echo "=========================================="

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: $(TARGET)
	@echo "Compilacao de testes executada com sucesso!"

format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

doc:
	doxygen Doxyfile

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf Doc/html
