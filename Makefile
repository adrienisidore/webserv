# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/03 14:09:31 by aisidore          #+#    #+#              #
#    Updated: 2025/10/08 12:20:23 by aisidore         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

FLAGS = -Wall -Wextra -Werror -g -std=c++98

INCLUDES = -I./includes/

SRC = src/main.cpp \
      src/Server.cpp \
	  src/serverReply.cpp \
	  src/Request.cpp \
	  src/Response.cpp \
	  src/Client.cpp

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
