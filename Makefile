# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/03 14:09:31 by aisidore          #+#    #+#              #
#    Updated: 2025/11/30 20:16:59 by gfontagn         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

FLAGS = -Wall -Wextra -Werror -g -std=c++98
#-fstandalone-debug

# For schoool
#INCLUDES = -I./includes/
# For me
INCLUDES = -I./includes/ -I/usr/include/c++/11 -I/usr/include/x86_64-linux-gnu/c++/11


SRC = src/main.cpp \
	  src/ConfigParser.cpp \
	  src/Request.cpp \
	  src/ServerMonitor.cpp \
	  src/Response.cpp \
	  src/TCPConnection.cpp \
	  src/HTTPcontent.cpp \
	  src/LocationConfig.cpp \
	  src/ServerConfig.cpp \
	  src/GlobalConfig.cpp \
	  src/autoconfig.cpp \
	  src/parsing.cpp \
	  src/CGI.cpp \
	  src/utils.cpp \
	  src/visualizer.cpp

OBJ = objets/

OSRCS = $(SRC:%.c=$(OBJ)%.o)

all: $(NAME)

$(NAME): $(OSRCS)
	c++ $(FLAGS) $(INCLUDES) $(OSRCS) -o $(NAME)

$(OBJ)%.o: src/%.cpp
	mkdir -p objets
	c++ $(FLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f objets/*.o

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
