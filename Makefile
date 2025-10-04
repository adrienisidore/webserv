# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/03 14:09:31 by aisidore          #+#    #+#              #
#    Updated: 2025/10/04 15:43:17 by gfontagn         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

FLAGS = -Wall -Wextra -Werror -g -std=c++98

INCLUDES = -I./includes/

SRC = src/main.cpp \
      src/Server.cpp

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
