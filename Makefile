NAME = GameOfLife

C++ = c++

FLAGS = -Wall -Wextra -Werror -Iinclude

LINKING =  -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl

SRCS = main.cpp GameLoop.cpp glad.cpp Game.cpp ShapeLoader.cpp

OBJS = $(SRCS:%.cpp=%.o)

all:$(NAME)

$(NAME):$(OBJS)
	$(C++) $(FLAGS) $(OBJS) $(LINKING) -o $(NAME)

%.o:%.cpp
	$(C++) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re